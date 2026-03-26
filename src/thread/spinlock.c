#include "debug.h"
#include <clod/thread.h>
#include "yield.h"

void clod_spinlock_lock(clod_spinlock *spinlock) {
	clod_spinlock lock = 0;
	while (!clod_atomic_cas(spinlock, &lock, 1)) {
		#if CLOD_DEBUG_THREAD
		if (lock != 0 && lock != 1) {
			debug(CLOD_DEBUG_THREAD, "called clod_spinlock_lock on an invalid value.");
		}
		#endif

		for (int i = 0; i < 500; i++) {
			clod_pause();
			lock = 0;
			if (clod_atomic_cas(spinlock, &lock, 1))
				return;
		}

		clod_yield();
		lock = 0;
	}
}

void clod_spinlock_unlock(clod_spinlock *spinlock) {
	#if CLOD_DEBUG_THREAD
	clod_spinlock expected = 1;
	if (!clod_atomic_cas(spinlock, &expected, 0)) {
		debug(CLOD_DEBUG_THREAD, "called clod_spinlock_unlock on an unlocked spinlock.");
	}
	#else
	clod_atomic_store(spinlock, 0);
	#endif
}
