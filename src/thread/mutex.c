#include "clod_config.h"
#include <clod/thread.h>
#include <assert.h>
#include <errno.h>

#define ZERO 0
#define TRANSITIONING 1
#define INITIALISED 2

static bool clod_mutex_impl_init(void *mutex);
static bool clod_mutex_impl_lock(void *mutex);
static void clod_mutex_impl_unlock(void *mutex);
static void clod_mutex_impl_destroy(void *mutex);

#if CLOD_HAVE_PTHREAD_H
#include <pthread.h>

static_assert(sizeof(pthread_mutex_t) <= sizeof(((clod_mutex *)nullptr)->_impl));
static_assert(alignof(pthread_mutex_t) <= alignof(clod_mutex));

bool clod_mutex_impl_init(void *mutex) {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	if (pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_PROTECT))
		pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

	#ifndef NDEBUG
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	#endif

	return pthread_mutex_init(mutex, nullptr) == 0;
}
static bool clod_mutex_impl_lock(void *mutex) {
	const int res = pthread_mutex_lock(mutex);
	assert(res != EDEADLK);
	return res == 0;
}
static void clod_mutex_impl_unlock(void *mutex) {
	#if NDEBUG
		pthread_mutex_unlock(mutex);
	#else
		assert(pthread_mutex_unlock(mutex) == 0);
	#endif
}
static void clod_mutex_impl_destroy(void *mutex) {
	#if NDEBUG
		pthread_mutex_destroy(mutex);
	#else
		assert(pthread_mutex_destroy(mutex) == 0);
	#endif
}

#elif CLOD_HAVE_THREADS_H
#include <threads.h>

static_assert(sizeof(mtx_t) <= sizeof(((clod_mutex *)nullptr)->_impl));
static_assert(alignof(mtx_t) <= alignof(clod_mutex));

bool clod_mutex_impl_init(void *mutex) {
	return mtx_init(mutex, mtx_plain) == thrd_success;
}
static bool clod_mutex_impl_lock(void *mutex) {
	return mtx_lock(mutex) == thrd_success;
}
static void clod_mutex_impl_unlock(void *mutex) {
	#if NDEBUG
		mtx_unlock(mutex);
	#else
		assert(mtx_unlock(mutex) == thrd_success);
	#endif
}
static void clod_mutex_impl_destroy(void *mutex) {
	mtx_destroy(mutex);
}

#endif

bool clod_mutex_lock(clod_mutex *mutex) {
	char state = clod_atomic_load(&mutex->_state);
	while (state != INITIALISED) {
		state = ZERO;
		if (clod_atomic_cas(&mutex->_state, &state, TRANSITIONING)) {
			if (!clod_mutex_impl_init(mutex)) {
				clod_atomic_store(&mutex->_state, ZERO);
				return false;
			}

			clod_atomic_store(&mutex->_state, INITIALISED);
			state = INITIALISED;
		} else {
			state = clod_atomic_load(&mutex->_state);
		}
	}

	return clod_mutex_impl_lock(mutex);
}

void clod_mutex_unlock(clod_mutex *mutex) {
	assert(atomic_load(&mutex->_state) == INITIALISED);
	clod_mutex_impl_unlock(mutex);
}

void clod_mutex_destroy(clod_mutex *mutex) {
	char state = clod_atomic_load(&mutex->_state);
	while (state != ZERO) {
		state = INITIALISED;
		if (clod_atomic_cas(&mutex->_state, &state, TRANSITIONING)) {
			clod_mutex_impl_destroy(mutex);
			clod_atomic_store(&mutex->_state, ZERO);
			return;
		}
		state = clod_atomic_load(&mutex->_state);
	}
}
