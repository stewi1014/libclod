#include "debug.h"
#include <clod/thread.h>

clod_mutex background_mutex = CLOD_MUTEX_INIT;
volatile bool has_background_mutex;

int try_lock_background_mutex(int, char **) {
	clod_mutex_lock(&background_mutex);
	has_background_mutex = true;
	clod_mutex_unlock(&background_mutex);
	return 0;
}

int thread_background(int, char[]) {
	has_background_mutex = false;
	clod_mutex_lock(&background_mutex);

	struct clod_process_opts proc_opts = {
		.type = CLOD_THREAD_BACKGROUND,
		.main = try_lock_background_mutex
	};

	auto res = clod_process_start(&proc_opts, nullptr);
	assert_fatal(CLOD_TEST, res == CLOD_PROCESS_OK, "Should be able to create thread");
	clod_timer(nullptr, 100 * 1000);

	
	assert_fatal(CLOD_TEST, !has_background_mutex, "Other thread should acquire our locked mutex");

	clod_mutex_unlock(&background_mutex);
	clod_timer(nullptr, 100 * 1000);

	assert_fatal(CLOD_TEST, has_background_mutex, "Other thread should acquire our unlocked mutex");
	return 0;
}
