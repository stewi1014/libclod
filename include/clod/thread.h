/**
 * @file thread.h
 * @defgroup thread Threading
 * @{
 * A thin compatability layer for some synchronisation primitives and threading tools.
 * Implemented to avoid a dependency on any one threading API.
 */
#ifndef LIBCLOD_THREAD_H
#define LIBCLOD_THREAD_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __GNUC__
#define clod_atomic_load(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define clod_atomic_cas(ptr, expected, desired) __atomic_compare_exchange_n(\
	ptr, expected, desired,\
	false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST\
)
#define clod_atomic_store(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#endif

#define CLOD_THREAD_BUFFER_MAX 4096

/**
 * Function called in the new thread.
 * @param[in] data Data passed to clod_thread. 16 aligned.
 * @param[in] data_size Size of \p data.
 */
typedef void clod_thread_func(void *data, size_t data_size);

/**
 * Create a new thread.
 * The approach is slightly different to normal threading APIs in that instead of a single pointer
 * being passed to the thread, a buffer of data is passed instead. The idea being that one can simply
 * pass a whole struct without much thought.
 * @param[in] func Function called in the new thread.
 * @param[in] name Name of the thread.
 * @param[in] data Buffer of data passed to \p func.
 * @param[in] data_size Size of \p data. Max CLOD_THREAD_BUFFER_MAX.
 * @return True on success. False on failure.
 * Failure is likely due to a lack of system resources.
 */
CLOD_API CLOD_NONNULL(1, 2)
bool clod_thread(clod_thread_func *func, const char *name, const void *data, size_t data_size);

#define CLOD_SPINLOCK_INIT 0

/** Simple spinlock. Should never be copied. */
typedef bool clod_spinlock;

/** Lock a spinlock.
 * @param spinlock The spinlock to lock. */
CLOD_API CLOD_NONNULL(1)
void clod_spinlock_lock(clod_spinlock *spinlock);

/** Unlock a spinlock.
 * @param spinlock The spinlock to lock. */
CLOD_API CLOD_NONNULL(1)
void clod_spinlock_unlock(clod_spinlock *spinlock);

/**
 * Wait until a given duration has passed since a point in time,
 * incrementing the time with the given duration on return.
 * @note Simply reusing the same time value will result in a timer implementation that will
 * try to catch up if it falls behind. To instead have the timer continue with a constant cadence
 * following a slowdown, one can do @code if (return_val > time) time = return_val; @endcode
 * after calling clod_timer.
 * @param[in,out] time Time in microseconds from an undefined epoch.
 * If \p time is null, the method simply waits for duration_us.
 * The duration is added to \p time.
 * @param[in] duration_us Duration after \p time to wait for.
 * If \p duration_ns is <= 0, it simply returns the current time.
 * @return The time at which the method was entered.
 */
CLOD_API
int64_t clod_timer(int64_t *time, int64_t duration_us);

/**
 * Mutex.
 * Semantics can differ depending on which concrete implementation is used,
 * but should always be a high quality mutex with no big surprises.
 */
typedef struct {
	union {
		char _impl[40];
		alignas(8) char _align;
	};
	char _state;
} clod_mutex;

/// Libclod's mutex wrapper always supports zero initialisation.
#define CLOD_MUTEX_INIT (clod_mutex){0}

/** Lock a mutex.
 * It does not support recursive locking.
 * @param[in] mutex Mutex to lock.
 * @return True on success, false on failure.
 * Failure is likely due to lack of resources. */
CLOD_API CLOD_NONNULL(1)
bool clod_mutex_lock(clod_mutex *mutex);

/** Unlock a mutex.
 * @param[in] mutex Mutex to unlock. */
CLOD_API CLOD_NONNULL(1)
void clod_mutex_unlock(clod_mutex *mutex);

/** Release resources associated with the mutex.
 * @param[in] mutex Mutex to destroy. */
CLOD_API CLOD_NONNULL(1)
void clod_mutex_destroy(clod_mutex *mutex);

/// Libclod's read-write mutex wrapper always supports zero initialisation.
#define CLOD_RWMUTEX_INIT (clod_rwmutex){0}

typedef struct {
	union {
		char _impl[56];
		alignas(8) char _align;
	};
	char _state;
} clod_rwmutex;

/** Read lock a read-write mutex.
 * It does not support recursive locking.
 * @param[in] rwmutex Read-write mutex to lock.
 * @return True on success, false on failure.
 * Failure is likely due to lack of resources. */
CLOD_API CLOD_NONNULL(1)
bool clod_rwmutex_rdlock(clod_rwmutex *rwmutex);

/** Read unlock a read-write mutex.
 * @param[in] rwmutex Read-write mutex to unlock. */
CLOD_API CLOD_NONNULL(1)
void clod_rwmutex_rdunlock(clod_rwmutex *rwmutex);

/** Write lock a read-write mutex.
 * It does not support recursive locking.
 * @param[in] rwmutex Read-write mutex to lock.
 * @return True on success, false on failure.
 * Failure is likely due to lack of resources. */
CLOD_API CLOD_NONNULL(1)
bool clod_rwmutex_wrlock(clod_rwmutex *rwmutex);

/** Write unlock a read-write mutex.
 * @param[in] rwmutex Read-write mutex to unlock.*/
CLOD_API CLOD_NONNULL(1)
void clod_rwmutex_wrunlock(clod_rwmutex *rwmutex);

/** Release resources associated with a read-write mutex.
 * It is safe to use on a zeroed mutex.
 * @param[in] rwmutex Read-write mutex to destroy. */
CLOD_API CLOD_NONNULL(1)
void clod_rwmutex_destroy(clod_rwmutex *rwmutex);

/** @} */
#endif