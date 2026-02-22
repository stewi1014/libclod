#include "clod_config.h"
#include <clod/thread.h>
#include <assert.h>
#include <errno.h>

#define ZERO 0
#define TRANSITIONING 1
#define INITIALISED 2

static bool clod_rwmutex_impl_init(void *mutex);
static bool clod_rwmutex_impl_rdlock(void *mutex);
static void clod_rwmutex_impl_rdunlock(void *mutex);
static bool clod_rwmutex_impl_wrlock(void *mutex);
static void clod_rwmutex_impl_wrunlock(void *mutex);
static void clod_rwmutex_impl_destroy(void *mutex);

#if CLOD_HAVE_PTHREAD_H
#include <pthread.h>

static_assert(sizeof(pthread_rwlock_t) <= sizeof(((clod_rwmutex *)nullptr)->_impl));
static_assert(alignof(pthread_rwlock_t) <= alignof(clod_rwmutex));

bool clod_rwmutex_impl_init(void *mutex) {
	pthread_rwlockattr_t attr;
	pthread_rwlockattr_init(&attr);

	#if CLOD_HAVE_PTHREAD_RWLOCKATTR_SETKIND
		pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	#endif

	return pthread_rwlock_init(mutex, &attr) == 0;
}

static bool clod_rwmutex_impl_rdlock(void *mutex) {
	const int res = pthread_rwlock_rdlock(mutex);
	assert(res != EDEADLK);
	return res == 0;
}

static void clod_rwmutex_impl_rdunlock(void *mutex) {
	#if NDEBUG
		pthread_rwlock_unlock(mutex);
	#else
		assert(pthread_rwlock_unlock(mutex) == 0);
	#endif
}

static bool clod_rwmutex_impl_wrlock(void *mutex) {
	const int res = pthread_rwlock_wrlock(mutex);
	assert(res != EDEADLK);
	return res == 0;
}

static void clod_rwmutex_impl_wrunlock(void *mutex) {
	#if NDEBUG
		pthread_rwlock_unlock(mutex);
	#else
		assert(pthread_rwlock_unlock(mutex) == 0);
	#endif
}

static void clod_rwmutex_impl_destroy(void *mutex) {
	#if NDEBUG
		pthread_rwlock_destroy(mutex);
	#else
		assert(pthread_rwlock_destroy(mutex) == 0);
	#endif
}

#endif

bool clod_rwmutex_rdlock(clod_rwmutex *rwmutex) {
	char state = clod_atomic_load(&rwmutex->_state);
	while (state != INITIALISED) {
		state = ZERO;
		if (clod_atomic_cas(&rwmutex->_state, &state, TRANSITIONING)) {
			if (!clod_rwmutex_impl_init(rwmutex)) {
				clod_atomic_store(&rwmutex->_state, ZERO);
				return false;
			}

			clod_atomic_store(&rwmutex->_state, INITIALISED);
			state = INITIALISED;
		} else {
			state = clod_atomic_load(&rwmutex->_state);
		}
	}

	return clod_rwmutex_impl_rdlock(rwmutex);
}

void clod_rwmutex_rdunlock(clod_rwmutex *rwmutex) {
	assert(atomic_load(&rwmutex->_state) == INITIALISED);
	clod_rwmutex_impl_rdunlock(rwmutex);
}

bool clod_rwmutex_wrlock(clod_rwmutex *rwmutex) {
	char state = clod_atomic_load(&rwmutex->_state);
	while (state != INITIALISED) {
		state = ZERO;
		if (clod_atomic_cas(&rwmutex->_state, &state, TRANSITIONING)) {
			if (!clod_rwmutex_impl_init(rwmutex)) {
				clod_atomic_store(&rwmutex->_state, ZERO);
				return false;
			}

			clod_atomic_store(&rwmutex->_state, INITIALISED);
			state = INITIALISED;
		} else {
			state = clod_atomic_load(&rwmutex->_state);
		}
	}

	return clod_rwmutex_impl_wrlock(rwmutex);
}

void clod_rwmutex_wrunlock(clod_rwmutex *rwmutex) {
	assert(atomic_load(&rwmutex->_state) == INITIALISED);
	clod_rwmutex_impl_wrunlock(rwmutex);
}

void clod_rwmutex_destroy(clod_rwmutex *rwmutex) {
	char state = clod_atomic_load(&rwmutex->_state);
	while (state != ZERO) {
		state = INITIALISED;
		if (clod_atomic_cas(&rwmutex->_state, &state, TRANSITIONING)) {
			clod_rwmutex_impl_destroy(rwmutex);
			clod_atomic_store(&rwmutex->_state, ZERO);
			return;
		} else {
			state = clod_atomic_load(&rwmutex->_state);
		}
	}
}
