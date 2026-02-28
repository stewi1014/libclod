#include "test.h"
#include <clod/thread.h>

clod_mutex mutex = CLOD_MUTEX_INIT;
volatile bool has_mutex;

int try_lock_mutex(int argc, char **argv) {
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
	test_check(res == CLOD_PROCESS_OK, "Should be able to create thread")
		return 1;
	clod_timer(nullptr, 100 * 1000);

	test_check(!has_mutex, "Other thread should not acquire our locked mutex")
		return 1;

	clod_mutex_unlock(&mutex);
	clod_timer(nullptr, 100 * 1000);

	test_check(has_mutex, "Other thread should acquire our unlocked mutex")
		return 1;

	return 0;
}
