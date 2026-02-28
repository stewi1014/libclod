#include <assert.h>
#include <string.h>

#include "clod_thread_config.h"
#include <clod/thread.h>

#include "thread_impl.h"

enum clod_process_result
clod_process_start(struct clod_process_opts *opts, clod_process *process_out) {
	struct clod_process_common **common = (struct clod_process_common**)process_out;

	#if CLOD_HAVE_PTHREAD
	if (opts->type == CLOD_THREAD) {
		auto res = clod_process_start_pthread(opts, common);
		if (res == CLOD_PROCESS_OK && common) (*common)->type = CLOD_THREAD;
		return res;
	}
	#elif CLOD_HAVE_STDTHREADS
	if (opts->type == CLOD_THREAD) {
		auto res = clod_process_start_stdthreads(opts, common);
		if (res == CLOD_PROCESS_OK && common) (*common)->type = CLOD_THREAD;
		return res;
	}
	#endif

	#if CLOD_HAVE_LINUX_SCHED
	if (opts->type == CLOD_DAEMON || opts->type == CLOD_THREAD_BACKGROUND) {
		auto res = clod_process_start_linux(opts, common);
		if (res == CLOD_PROCESS_OK && common) (*common)->type = opts->type;
		return res;
	}
	#endif

	return CLOD_PROCESS_UNSUPPORTED;
}

enum clod_process_result
clod_process_join(const clod_process process) {
	struct clod_process_common *common = (struct clod_process_common*)process;

	#if CLOD_HAVE_PTHREAD
	if (common->type == CLOD_THREAD)
		return clod_process_join_pthread(common);
	#elif CLOD_HAVE_STDTHREADS
	if (common->type == CLOD_THREAD)
		return clod_process_join_stdthreads(common);
	#endif

	#if CLOD_HAVE_LINUX_SCHED
	if (common->type == CLOD_DAEMON || common->type == CLOD_THREAD_BACKGROUND)
		return clod_process_join_linux(common);
	#endif

	return CLOD_PROCESS_INVALID;
}

size_t args_size(const struct clod_process_args *args) {
	const size_t head_size = sizeof(struct clod_process_args);
	const size_t sizes_size = sizeof(args->arg_sizes[0]) * (size_t)args->arg_count;
	const size_t vector_size = sizeof(args->arg_vector[0]) * ((size_t)args->arg_count + 1);

	const size_t sizes_offset = ALIGN(head_size, alignof(typeof(args->arg_sizes[0])));
	const size_t vector_offset = ALIGN(sizes_offset + sizes_size, alignof(typeof(args->arg_vector[0])));
	const size_t arg_offset = ALIGN(vector_offset + vector_size, 16);

	size_t size = arg_offset;
	for (int i = 0; i < args->arg_count; i++) {
		if (args->arg_vector[i] == nullptr)
			continue;

		if (args->arg_sizes) {
			size += ALIGN(args->arg_sizes[i] + 1, 16);
		} else {
			size += ALIGN(strlen(args->arg_vector[i]) + 1, 16);
		}
	}

	assert(size % 16 == 0);
	return size;
}
void args_copy(struct clod_process_args *dst, const struct clod_process_args *src) {
	assert((uintptr_t)dst % 16 == 0);

	const size_t head_size = sizeof(struct clod_process_args);
	const size_t sizes_size = sizeof(src->arg_sizes[0]) * (size_t)src->arg_count;
	const size_t vector_size = sizeof(src->arg_vector[0]) * ((size_t)src->arg_count + 1);

	const size_t sizes_offset = ALIGN(head_size, alignof(typeof(src->arg_sizes[0])));
	const size_t vector_offset = ALIGN(sizes_offset + sizes_size, alignof(typeof(src->arg_vector[0])));
	const size_t arg_offset = ALIGN(vector_offset + vector_size, 16);

	dst->arg_count = src->arg_count;
	dst->arg_sizes = (size_t*)((char*)dst + sizes_offset);
	dst->arg_vector = (char**)((char*)dst + vector_offset);

	size_t size = arg_offset;
	for (int i = 0; i < src->arg_count; i++) {
		size_t arg_size;
		if (src->arg_sizes)
			arg_size = src->arg_sizes[i];
		else if (src->arg_vector[i])
			arg_size = strlen(src->arg_vector[i]);
		else
			arg_size = 0;

		dst->arg_sizes[i] = arg_size;
		if (src->arg_vector[i]) {
			dst->arg_vector[i] = (char*)dst + size;
			memcpy(dst->arg_vector[i], src->arg_vector[i], arg_size);
			dst->arg_vector[i][arg_size] = '\0';
			size += ALIGN(arg_size + 1, 16);
		} else {
			dst->arg_vector[i] = nullptr;
		}
	}

	if (!src->arg_vector)
		dst->arg_vector = nullptr;
	else
		dst->arg_vector[src->arg_count] = nullptr;
}
