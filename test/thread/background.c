#include "test.h"
#include <clod/thread.h>

clod_mutex background_mutex = CLOD_MUTEX_INIT;
volatile bool has_background_mutex;

int try_lock_background_mutex(int argc, char **argv) {
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
	test_check(res == CLOD_PROCESS_OK, "Could not create CLOD_THREAD_BACKGROUND thread")
		return 1;
	clod_timer(nullptr, 100 * 1000);

	test_check(!has_background_mutex, "Other thread should not acquire our locked mutex")
		return 1;

	clod_mutex_unlock(&background_mutex);
	clod_timer(nullptr, 100 * 1000);

	test_check(has_background_mutex, "Other thread should acquire our unlocked mutex")
		return 1;

	return 0;
}
