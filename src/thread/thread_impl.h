#ifndef LIBCLOD_THREAD_IMPL_H
#define LIBCLOD_THREAD_IMPL_H

#include "config.h"
#include <clod/thread.h>

#define ALIGN(size, alignment) (((size) + (alignment) - 1) &~ (typeof(size))((alignment) - 1))

struct clod_process_args {
	int arg_count;
	size_t *arg_sizes;
	char **arg_vector;
};

size_t args_size(const struct clod_process_args *args);
void args_copy(struct clod_process_args *dst, const struct clod_process_args *src);

struct clod_process_common {
	enum clod_process_type type;
};

#if CLOD_HAVE_PTHREAD
	enum clod_process_result clod_process_start_pthread(struct clod_process_opts *opts, struct clod_process_common **process_out);
	enum clod_process_result clod_process_join_pthread(struct clod_process_common *process);
#endif

#if CLOD_HAVE_STDTHREADS
	enum clod_process_result clod_process_start_stdthreads(struct clod_process_opts *opts, struct clod_process_common **process_out);
	enum clod_process_result clod_process_join_stdthreads(struct clod_process_common *process);
#endif

#if CLOD_HAVE_LINUX_SCHED
	enum clod_process_result clod_process_start_linux(struct clod_process_opts *opts, struct clod_process_common **process_out);
	enum clod_process_result clod_process_join_linux(struct clod_process_common *process);
#endif

#endif
