#include <stdio.h>

#include "clod_thread_config.h"
#include "thread_impl.h"
#include <threads.h>
#include <stdlib.h>

struct clod_process_stdthreads {
	struct clod_process_common common;
	thrd_t thrd;
};

struct thread_args {
	clod_process_main *main;
};

int clod_process_stdthreads_main(void *ptr) {
	struct thread_args *thread_args = ptr;
	auto args = (struct clod_process_args *)((char*)ptr + ALIGN(sizeof(struct thread_args), 16));
	thread_args->main(args->arg_count, args->arg_vector);
	free(thread_args);
	return 0;
}

enum clod_process_result clod_process_start_stdthreads(
	struct clod_process_opts *opts,
	struct clod_process_common **process_out
) {
	if (!opts->main) {
		#if CLOD_DEBUG_THREAD
		fprintf(stderr, "libclod: clod_process_start_pthread: no thread main function given.\n");
		#endif
		return CLOD_PROCESS_INVALID;
	}
	if (opts->type != CLOD_THREAD) {
		#if CLOD_DEBUG_THREAD
		fprintf(stderr, "libclod: clod_process_start_stdthreads: given %d, but only CLOD_THREAD (1) is supported here.\n", opts->type);
		#endif
		return CLOD_PROCESS_INVALID;
	}

	struct clod_process_args args_in = {
		.arg_count = opts->arg_count,
		.arg_sizes = opts->arg_sizes,
		.arg_vector = opts->arg_vector
	};

	char *data = malloc(ALIGN(sizeof(struct thread_args), 16) + args_size(&args_in));
	auto thread_args = (struct thread_args *)data;
	auto args = (struct clod_process_args *)(data + ALIGN(sizeof(struct thread_args), 16));

	thread_args->main = opts->main;
	args_copy(args, &args_in);

	thrd_t thrd;
	const int res = thrd_create(&thrd, clod_process_stdthreads_main, data);
	if (res != thrd_success) {
		free(data);
		#if CLOD_DEBUG_THREAD
		fprintf(stderr, "libclod: pthread_create: %d\n", res);
		#endif
		if (res == thrd_nomem) return CLOD_PROCESS_NO_MEMORY;
		return CLOD_PROCESS_INVALID;
	}

	if (process_out) {
		struct clod_process_stdthreads *process = malloc(sizeof(struct clod_process_stdthreads));
		process->common.type = CLOD_THREAD;
		process->thrd = thrd;
		*process_out = &process->common;
	} else {
		thrd_detach(thrd);
	}

	return CLOD_PROCESS_OK;
}

enum clod_process_result clod_process_join_stdthreads(struct clod_process_common *process) {
	auto stdthreads_process = (struct clod_process_stdthreads*)process;
	const int res = thrd_join(stdthreads_process->thrd, nullptr);
	free(stdthreads_process);
	if (res != thrd_success) return CLOD_PROCESS_INVALID;
	return CLOD_PROCESS_OK;
}
