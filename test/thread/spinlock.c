#include <clod/thread.h>

clod_spinlock spinlock = CLOD_SPINLOCK_INIT;
bool has_spinlock;

void try_lock_spinlock(void *, size_t) {
	clod_spinlock_lock(&spinlock);
	has_spinlock = true;
	clod_spinlock_unlock(&spinlock);
}

int thread_spinlock() {
	has_spinlock = false;
	clod_spinlock_lock(&spinlock);

	for (int i = 0; i < 32; i++)
		clod_thread(try_lock_spinlock, "Test thread", nullptr, 0);
	clod_timer(nullptr, 100 * 1000);

	if (has_spinlock) return 1;

	clod_spinlock_unlock(&spinlock);
	clod_timer(nullptr, 100 * 1000);

	if (!has_spinlock) return 1;
	return 0;
}
