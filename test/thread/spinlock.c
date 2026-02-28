#include "test.h"
#include <clod/thread.h>

clod_spinlock spinlock = CLOD_SPINLOCK_INIT;
bool has_spinlock;

int try_lock_spinlock(int argc, char **argv) {
	clod_spinlock_lock(&spinlock);
	has_spinlock = true;
	clod_spinlock_unlock(&spinlock);
	return 0;
}

int thread_spinlock(int, char[]) {
	has_spinlock = false;
	clod_spinlock_lock(&spinlock);

	struct clod_process_opts proc_opts = {
		.type = CLOD_THREAD,
		.main = try_lock_spinlock
	};

	auto res = clod_process_start(&proc_opts, nullptr);
	test_check(res == CLOD_PROCESS_OK, "Should be able to create thread")
		return 1;

	clod_timer(nullptr, 100 * 1000);

	test_check(!has_spinlock, "Process should not have locked our locked spinlock")
		return 1;

	clod_spinlock_unlock(&spinlock);
	clod_timer(nullptr, 100 * 1000);

	test_check(has_spinlock, "Process should have locked our unlocked spinlock")
		return 1;
	return 0;
}
