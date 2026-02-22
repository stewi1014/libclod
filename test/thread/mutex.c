#include "test.h"
#include <clod/thread.h>

clod_mutex mutex = CLOD_MUTEX_INIT;
volatile bool has_mutex;

void try_lock_mutex(void *, size_t) {
	clod_mutex_lock(&mutex);
	has_mutex = true;
	clod_mutex_unlock(&mutex);
}

int thread_mutex() {
	has_mutex = false;
	clod_mutex_lock(&mutex);

	for (int i = 0; i < 32; i++)
		clod_thread(try_lock_mutex, "Test thread", nullptr, 0);
	clod_timer(nullptr, 100 * 1000);

	test_check(!has_mutex, "Other thread should not acquire our locked mutex")
		return 1;

	clod_mutex_unlock(&mutex);
	clod_timer(nullptr, 100 * 1000);

	test_check(has_mutex, "Other thread should acquire our unlocked mutex")
		return 1;

	clod_mutex_destroy(&mutex);
	return 0;
}
