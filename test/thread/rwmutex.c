#include "test.h"
#include <clod/thread.h>

clod_rwmutex rwmutex = CLOD_RWMUTEX_INIT;
volatile bool has_rwmutex;

void try_lock_rwmutex(void *, size_t) {
	clod_rwmutex_wrlock(&rwmutex);
	has_rwmutex = true;
	clod_rwmutex_wrunlock(&rwmutex);
}

int thread_rwmutex() {
	has_rwmutex = false;
	clod_rwmutex_rdlock(&rwmutex);

	for (int i = 0; i < 32; i++)
		clod_thread(try_lock_rwmutex, "Test thread", nullptr, 0);
	clod_timer(nullptr, 100 * 1000);

	test_check(!has_rwmutex, "Other thread should not write acquire our read locked rwmutex")
		return 1;

	clod_rwmutex_rdunlock(&rwmutex);
	clod_timer(nullptr, 100 * 1000);

	test_check(has_rwmutex, "Other thread should write acquire our unlocked rwmutex")
		return 1;

	clod_rwmutex_destroy(&rwmutex);
	return 0;
}
