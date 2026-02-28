#include "clod_config.h"
#include <clod/thread.h>
#include "thread_impl.h"
#include <assert.h>
#include <errno.h>
#include <linux/sched.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/random.h>


#define DEFAULT_STACK_SIZE (3 * 1024)

struct clod_process_linux {
	struct clod_process_common common;
	pid_t child_tid;
};

struct stack_header;

/**
 * Bootstrap a new execution environment.
 * For the parent, the method quickly returns having only made the clone syscall.
 * For the child, it calls clod_execution_main
 *
 * @param[in] args Arguments passed to clone3
 * @param[in] stack Pointer to the stack at the offset where the header starts.
 * @return Result of clone3 syscall.
 */
long clod_execution_bootstrap(const struct clone_args *args, struct stack_header *stack);
int clod_execution_main(const struct stack_header *stack);

#if CLOD_HAVE_X86_64

struct stack_header {
	void *stack;
	size_t stack_size;
	int *errno_location;
	int error_number;
	clod_process_main *main;
	uint64_t stack_guard;
	struct clod_process_args *args;
};

static_assert(offsetof(struct stack_header, stack_guard) == 0x28);
static_assert(offsetof(struct stack_header, errno_location) == 0x10);

long clod_execution_bootstrap(const struct clone_args *args, struct stack_header *stack) {
	long result;
	__asm volatile(
		"mov %[args], %%rdi\n"
		"mov %[args_size], %%rsi\n"
		"mov %[clone3], %%eax\n"
		"syscall\n"
		"cmp $0, %%rax\n"
		"jne parent\n"

		// We are the new process.
		"push %[stack_ptr]\n"
		"push %[stack_size]\n"
		"mov %[stack], %%rdi\n"
		"call *%[main]\n"

		// Save return value for later
		"mov %%rax, %%rbx\n"

		// Free our own stack from underneath us.
		"pop %%rsi\n"
		"pop %%rdi\n"
		"mov %[munmap], %%eax\n"
		"syscall\n"

		// Call exit with main return value.
		"mov %%rbx, %%rdi\n"
		"mov %[exit], %%eax\n"
		"syscall\n"
		"ud2\n"

	"parent:\n"
		"mov %%rax, %[result]\n"
		:
			[result] "=r" (result)
		:
			[args] "r" ((long)args),
			[stack] "r" ((long)stack),
			[stack_ptr] "r" ((long)stack->stack),
			[stack_size] "r" ((long)stack->stack_size),
			[main] "r" ((long)&clod_execution_main),

			[args_size] "i" ((long)sizeof(struct clone_args)),
			[clone3] "i" (SYS_clone3),
			[munmap] "i" (SYS_munmap),
			[exit] "i" (SYS_exit)
		:
			// More are actually clobbered i.e. rbx, but it doesn't matter since we're never
			// going to return if we're the child.
			"rdi", "rsi", "rax", "rcx", "r11", "memory", "cc"
	);
	return result;
}

#else
#error "Process bootstrap method not implemented for this architecture"
#endif

int clod_execution_main(const struct stack_header *stack) {
	return stack->main(stack->args->arg_count, stack->args->arg_vector);
}

void *get_stack(size_t *stack_size_out) {
	pid_t tid = gettid();
	char buff[256];
	snprintf(buff, sizeof(buff), "/proc/self/task/%d/maps", tid);
	FILE *file = fopen(buff, "r");
	if (!file) file = fopen("/proc/self/maps", "r");
	if (!file) return nullptr;

	uintptr_t stack_start = 0, stack_end = 0;

	while (fgets(buff, sizeof(buff), file)) {
		if (strstr(buff, "[stack]")) {
			char *end;
			stack_start = strtoul(buff, &end, 16);
			stack_end = strtoul(end + 1, &end, 16);
			break;
		}
	}

	fclose(file);

	if (stack_start == 0 || stack_end == 0) return nullptr;
	*stack_size_out = stack_end - stack_start;
	return (void*)stack_start;
}

enum clod_process_result clod_process_start_linux(struct clod_process_opts *opts, struct clod_process_common **process_out) {
	if (!opts->main)
		return CLOD_PROCESS_INVALID;
	if (opts->type != CLOD_DAEMON && opts->type != CLOD_THREAD && opts->type != CLOD_THREAD_BACKGROUND)
		return CLOD_PROCESS_INVALID;

	size_t stack_header_size = sizeof(struct stack_header);
	size_t stack_args_size = args_size(&(struct clod_process_args){
		.arg_count = opts->arg_count,
		.arg_sizes = opts->arg_sizes,
		.arg_vector = opts->arg_vector
	});

	size_t stack_header_offset = 0;
	size_t stack_args_offset = ALIGN(stack_header_offset + stack_header_size, 16);

	size_t stack_offset = stack_args_offset + stack_args_size;
	size_t stack_size = stack_offset;
	stack_size += opts->stack_size ? opts->stack_size : DEFAULT_STACK_SIZE;
	stack_size = ALIGN(stack_size + 4096, 4096);

	char *stack = mmap(
		nullptr, stack_size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK,
		-1, 0
	);
	if (stack == MAP_FAILED)
		return CLOD_PROCESS_NO_MEMORY;
	mprotect(stack, 4096, PROT_NONE);

	char *stack_data = stack + stack_size - stack_offset;
	auto header = (struct stack_header *)(stack_data + stack_header_offset);
	auto args = (struct clod_process_args *)(stack_data + stack_args_offset);

	header->main = opts->main;
	header->errno_location = &header->error_number;
	header->stack = stack;
	header->stack_size = stack_size;
	header->args = args;
	getrandom(&header->stack_guard, sizeof(header->stack_guard), 0);
	header->stack_guard &= ~(uint64_t)0xFF;
	args_copy(args, &(struct clod_process_args){
		.arg_count = opts->arg_count,
		.arg_sizes = opts->arg_sizes,
		.arg_vector = opts->arg_vector
	});

	struct clone_args clone_args = {0};
	clone_args.stack = (__u64)stack;
	clone_args.stack_size = stack_size - stack_offset;
	clone_args.tls = (__u64)stack_data; // in the guard page.

	switch (opts->type) {
		case CLOD_THREAD: clone_args.flags =
			CLONE_VM | CLONE_FILES | CLONE_THREAD | CLONE_SIGHAND | CLONE_FS | CLONE_IO | CLONE_SYSVSEM | CLONE_SETTLS;
			break;
		case CLOD_THREAD_BACKGROUND: clone_args.flags =
			CLONE_VM | CLONE_FILES | CLONE_SIGHAND | CLONE_FS | CLONE_IO | CLONE_SYSVSEM | CLONE_SETTLS;
			break;
		case CLOD_DAEMON: clone_args.flags =
			CLONE_VM | CLONE_FILES | CLONE_CLEAR_SIGHAND | CLONE_SETTLS;
			break;
	}

	struct clod_process_linux *process = nullptr;
	if (process_out) {
		process = malloc(sizeof(struct clod_process_linux));
		clone_args.child_tid = (__u64)&process->child_tid;
		clone_args.flags |= CLONE_CHILD_SETTID;
		*process_out = &process->common;
	}

	long res = clod_execution_bootstrap(&clone_args, header);
	if (res < 0) {
		if (process) free(process);
		munmap(stack, stack_size);
		switch (errno) {
			case ENOMEM: return CLOD_PROCESS_NO_MEMORY;
			default: return CLOD_PROCESS_INVALID;
		}
	}

	return CLOD_PROCESS_OK;
}

enum clod_process_result clod_process_join_linux(struct clod_process_common *process) {
	auto linux_process = (struct clod_process_linux*)process;
	syscall(SYS_futex_wait, &linux_process->child_tid, 0, nullptr, nullptr, 0);
	return CLOD_PROCESS_OK;
}
