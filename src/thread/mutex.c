#include <clod/thread.h>
#include <clod/sys/futex.h>
#include "clod/debug.h"
#include "yield.h"

#define FUTEX_TIMEOUT_US (1000 * 1000)

#define ZERO 0
#define LOCKED 1
#define WAITER 2

void clod_mutex_lock(clod_mutex *mutex) {
	while (1) {
		int lock = clod_atomic_load(mutex);
		while (lock & LOCKED) {
			for (int i = 0; i < 500 && lock & LOCKED; i++) {
				clod_pause();
				lock = clod_atomic_load(mutex);
			}

			if (lock & LOCKED && clod_atomic_cas(mutex, &lock, lock + WAITER)) {
				clod_futex_wait(mutex, lock + WAITER, FUTEX_TIMEOUT_US);
				lock = clod_atomic_add(mutex, -WAITER);
			}
		}

		if (clod_atomic_cas(mutex, &lock, lock + 1))
			return;
	}
}

void clod_mutex_unlock(clod_mutex *mutex) {
	#if CLOD_DEBUG_THREAD
	if (!(clod_atomic_load(mutex) & 1)) {
		debug(CLOD_DEBUG_THREAD, "called clod_mutex_unlock on an unlocked mutex.");
	}
	#endif

	int lock = clod_atomic_add(mutex, -1);
	if (lock) {
		clod_futex_wake_one(mutex);
	}
}
