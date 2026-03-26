#include "config.h"
#include "syscall.h"
#include <clod/sys/sys.h>
#include <linux/time.h>
#include <linux/unistd.h>

#ifdef CLOD_HAVE_X86_64

#pragma GCC diagnostic push
#if __clang__
#pragma GCC diagnostic ignored "-Wlanguage-extension-token"
#endif
#pragma GCC diagnostic ignored "-Wunused-parameter"

__attribute__((naked))
static long syscall_impl(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
	asm volatile(
		"mov %%rcx, %%r10\n"
		"syscall\n"
		"ret"
		: : : "rax", "rcx", "r11", "memory"
	);
}

static long syscall(int number, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
	asm volatile("mov %[number], %%eax" : : [number] "r" (number) : "rax");
	return syscall_impl(arg1, arg2, arg3, arg4, arg5, arg6);
}

#pragma GCC diagnostic pop

#else
#error "Linux syscalls not implemented on this architecture"
#endif

int syscall_futex_wait(const int *addr, const int expected, struct timespec *timeout) {
	return (int)syscall(__NR_futex, (long)addr, __NR_futex_wait, expected, (long)timeout, 0, 0);
}

int syscall_futex_wake(const int *addr, const int num) {
	return (int)syscall(__NR_futex, (long)addr, __NR_futex_wake, num, 0, 0, 0);
}

ssize_t syscall_read(const int fd, void *buff, const size_t size) {
	return (ssize_t)syscall(__NR_read, fd, (long)buff, (long)size, 0, 0, 0);
}

ssize_t syscall_write(const int fd, const void *buff, const size_t size) {
	return (ssize_t)syscall(__NR_write, fd, (long)buff, (long)size, 0, 0, 0);
}

int syscall_close(const int fd) {
	return (int)syscall(__NR_close, fd, 0, 0, 0, 0, 0);
}

void *syscall_mmap(void *addr, const size_t length, const int prot, const int flags, const int fd, const off_t offset) {
	return (void*)syscall(__NR_mmap, (long)addr, (long)length, prot, flags, fd, offset);
}

int syscall_munmap(void *addr, const size_t length) {
	return (int)syscall(__NR_munmap, (long)addr, (long)length, 0, 0, 0, 0);
}

void clod_exit(const int code) {
	syscall(__NR_exit, code, 0, 0, 0, 0, 0);
	__builtin_unreachable();
}
