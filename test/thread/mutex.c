#include "debug.h"
#include <clod/thread.h>

clod_mutex mutex = CLOD_MUTEX_INIT;
volatile bool has_mutex;

int try_lock_mutex(int, char **) {
	clod_mutex_lock(&mutex);
	has_mutex = true;
	clod_mutex_unlock(&mutex);
	return 0;
}

int thread_mutex(int, char[]) {
	has_mutex = false;
	clod_mutex_lock(&mutex);

	struct clod_process_opts proc_opts = {
		.type = CLOD_THREAD,
		.main = try_lock_mutex
	};

	auto res = clod_process_start(&proc_opts, nullptr);
	assert_fatal(CLOD_TEST, res == CLOD_PROCESS_OK, "Should be able to create thread");
	clod_timer(nullptr, 100 * 1000);

	assert_fatal(CLOD_TEST, !has_mutex, "Other thread should acquire our locked mutex");

	clod_mutex_unlock(&mutex);
	clod_timer(nullptr, 100 * 1000);

	assert_fatal(CLOD_TEST, has_mutex, "Other thread should acquire our unlocked mutex");
	return 0;
}
