#include "clod_thread_config.h"
#include "clod/debug.h"
#include <clod/thread.h>
#include "yield.h"

#define ZERO 0
#define BUSY 1
#define DONE 2

bool clod_once_do(clod_once *once) {
	while (1) {
		clod_once state = clod_atomic_load(once);

		#if CLOD_DEBUG_THREAD
		if (state != ZERO && state != BUSY && state != DONE) {
			debug(CLOD_DEBUG_THREAD, "called clod_once_do with an invalid value.");
		}
		#endif

		if (state == ZERO && clod_atomic_cas(once, &state, BUSY))
			return true;

		while (state == BUSY) {
			for (int i = 0; i < 500 && state == BUSY; i++) {
				clod_pause();
				state = clod_atomic_load(once);
			}

			clod_yield();
			state = clod_atomic_load(once);
		}

		if (state == DONE)
			return false;
	}
}

void clod_once_done(clod_once *once) {
	#if CLOD_DEBUG_THREAD
	clod_once expected = BUSY;
	if (!clod_atomic_cas(once, &expected, DONE)) {
		debug(CLOD_DEBUG_THREAD, "called clod_once_done with an invalid value.");
	}
	#else
	clod_atomic_store(once, DONE);
	#endif
}
