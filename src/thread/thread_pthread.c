#include "clod_thread_config.h"
#include "thread_impl.h"
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

struct clod_process_pthread {
	struct clod_process_common common;
	pthread_t thread;
};

struct thread_args {
	clod_process_main *main;
};

void *clod_process_pthread_main(void *ptr) {
	struct thread_args *thread_args = ptr;
	auto args = (struct clod_process_args *)((char*)ptr + ALIGN(sizeof(struct thread_args), 16));
	thread_args->main(args->arg_count, args->arg_vector);
	free(thread_args);
	return nullptr;
}

enum clod_process_result clod_process_start_pthread(
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
		fprintf(stderr, "libclod: clod_process_start_pthread: given %d, but only CLOD_THREAD (1) is supported here.\n", opts->type);
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

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, process_out ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
	if (opts->stack_size)
		pthread_attr_setstacksize(&attr, opts->stack_size);

	pthread_t thread;
	const int res = pthread_create(&thread, &attr, clod_process_pthread_main, data);
	if (res != 0) {
		free(data);
		#if CLOD_DEBUG_THREAD
		fprintf(stderr, "libclod: pthread_create: %d\n", res);
		#endif
		if (errno == EAGAIN) return CLOD_PROCESS_NO_MEMORY;
		return CLOD_PROCESS_INVALID;
	}

	if (process_out) {
		struct clod_process_pthread *process = malloc(sizeof(struct clod_process_pthread));
		process->common.type = CLOD_THREAD;
		process->thread = thread;
		*process_out = &process->common;
	} else {
		pthread_detach(thread);
	}

	return CLOD_PROCESS_OK;
}

enum clod_process_result clod_process_join_pthread(struct clod_process_common *process) {
	auto pthread_process = (struct clod_process_pthread*)process;
	const int res = pthread_join(pthread_process->thread, nullptr);
	free(pthread_process);
	if (res != 0) return CLOD_PROCESS_INVALID;
	return CLOD_PROCESS_OK;
}
