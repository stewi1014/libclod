#include "clod_config.h"
#include <clod/thread.h>
#include "yield.h"

void clod_spinlock_lock(clod_spinlock *spinlock) {
	clod_spinlock expected;
	for (int i = 0; i < 500; i++) {
		expected = false;
		if (clod_atomic_cas(spinlock, &expected, true)) return;
		clod_pause();
	}

	do {
		clod_yield();
		expected = false;
	} while (!clod_atomic_cas(spinlock, &expected, true));
}

void clod_spinlock_unlock(clod_spinlock *spinlock) {
	clod_atomic_store(spinlock, 0);
}
