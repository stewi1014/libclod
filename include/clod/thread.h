/**
 * @file thread.h
 * @defgroup thread Threading
 * @{
 * Threading API. Implements threading from scratch, but since doing so forfeits
 * the use of libc, also wraps libc threading APIs.
 */
#ifndef LIBCLOD_THREAD_H
#define LIBCLOD_THREAD_H

#ifdef __GNUC__
	#define clod_atomic_load(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
	#define clod_atomic_cas(ptr, expected, desired) __atomic_compare_exchange_n(\
		ptr, expected, desired,\
		false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST\
	)
	#define clod_atomic_store(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
	#define clod_atomic_add(ptr, val) __atomic_add_fetch(ptr, val, __ATOMIC_SEQ_CST)
#endif

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

enum clod_process_type {
	/// The most typical kind of "thread", implemented using a standard threading API
	/// if one is found, with a fallback to libclod's own threading implementation if not.
	/// It shares memory, files, signals and other OS resources and contexts.
	/// Libclod explicitly avoids re-implementing anything with this type to
	/// facilitate compatability with runtimes that make assumptions about the execution
	/// environment's context that may be broken by a custom implementation.
	CLOD_THREAD = 1,

	/// Similar to CLOD_THREAD, but receives a new PID.
	/// It shares memory, files, signals and other OS resources and contexts.
	/// When the parent process dies it dies too, but it is not interrupted by the debugger
	/// or other signals. It is intended for use as a background task runner that is tied to
	/// the calling process's lifetime but otherwise wants autonomy.
	///
	///	@note Libclod does not attempt to replicate the exact thread-local storage data structures
	///	that any libc implementation might be using, and as such the execution environment is
	///	incompatible with libc. The thread local storage (i.e. FS register on x86_64) is explicitly
	///	pointed to invalid memory to reflect this.
	CLOD_THREAD_BACKGROUND = 2,

	/// Doesn't fall into the common concepts of a thread or process.
	/// Shares memory and open file handles, but does not share signals or other OS resources.
	/// The new execution environment attempts to be as thread-ish as possible, but receives a
	/// new PID and does not exit when the parent does. It is intended for use as a watchdog or
	/// background task runner.
	///
	///	@note Libclod does not attempt to replicate the exact thread-local storage data structures
	///	that any libc implementation might be using, and as such the execution environment is
	///	incompatible with libc. The thread local storage (i.e. FS register on x86_64) is explicitly
	///	pointed to invalid memory to reflect this.
	///
	/// @note When the parent dies, it may leave shared memory in an inconsistent state.
	/// <b>Extreme care</b> must be taken to ensure correctness if dealing with shared data.
	/// One should note that many C stdlib methods deal with shared memory internally, and
	/// larger runtimes (i.e. Golang, JVM) have no chance of functioning following the parent's
	/// death. In general, one would want to set up the child to receive a signal on the parent's
	/// death, and only perform some minimal operations before exiting itself.
	CLOD_DAEMON = 3,
};

enum clod_process_result {
	CLOD_PROCESS_OK = 0,
	CLOD_PROCESS_INVALID = 1,
	CLOD_PROCESS_NO_MEMORY = 2,
	CLOD_PROCESS_UNSUPPORTED = 3
};

typedef int clod_process_main(int argc, char **argv);

struct clod_process_opts {
	/// Type of process to create.
	enum clod_process_type type;

	/// Size of stack that the process shall have available to it. The actual stack size will
	/// likely be larger to accommodate arguments and other metadata.
	size_t stack_size;

	/// Process entry point.
	clod_process_main *main;

	/// Number of arguments passed to \p main.
	int arg_count;

	/// Arguments passed to \p main.
	/// They are copied and aligned to 16.
	char **arg_vector;

	/// Size of each argument in \p argv.
	/// If non-null, \p argv is interpreted as an array of generic data (i.e. not C strings).
	size_t *arg_sizes;

	/// Name of the process.
	/// Not always supported.
	const char *name;
};

typedef uintptr_t clod_process;

/**
 * Create a new execution environment.
 * @param[in] opts Configuration options.
 * @param[out] process_out (nullable) Handle to the new execution environment.
 * @return Result of the operation.
 */
CLOD_API CLOD_NONNULL(1)
enum clod_process_result
clod_process_start(struct clod_process_opts *opts, clod_process *process_out);

/** Wait for an execution environment to stop.
 * @param[in] process Execution environment to wait for.
 * @return Result of the operation. */
CLOD_API
enum clod_process_result
clod_process_join(clod_process process);

#define CLOD_SPINLOCK_INIT 0

/** Simple spinlock. Should never be copied. */
typedef char clod_spinlock;

/** Lock a spinlock.
 * @param spinlock The spinlock to lock. */
CLOD_API CLOD_NONNULL(1)
void clod_spinlock_lock(clod_spinlock *spinlock);

/** Unlock a spinlock.
 * @param spinlock The spinlock to lock. */
CLOD_API CLOD_NONNULL(1)
void clod_spinlock_unlock(clod_spinlock *spinlock);

#define CLOD_ONCE_INIT 0

#define CLOD_ONCE(once) for (bool _do_once = clod_once_do(once); _do_once; clod_once_done(once), _do_once = false)

/** Perform a function once and only once. */
typedef char clod_once;

/** Returns true only once. All other calls block until
 * clod_once_done is called following the return of true.
 * @param[in] once Meeting point for threads.
 * @return True for the first call, false for all subsequent calls. */
CLOD_API CLOD_NONNULL(1)
bool clod_once_do(clod_once *once);

/** Mark the value as done.
 * @param[in] once Meeting point for threads. */
CLOD_API CLOD_NONNULL(1)
void clod_once_done(clod_once *once);

/**
 * Multi-use time function. Can be used to get the current time, wait for a
 * specific time in the future, wait some duration from the current time
 * or easily implement a repeating task that avoids time drift.
 *
 * @code
 * for (int64_t time = clod_timer(nullptr, 0); true; clod_timer(&time, PERIOD_US)) {
 *     do_work();
 * }
 * @endcode
 *
 * @note Simply reusing the same time value will result in a timer implementation that will
 * not drift in theory, but in practice a task might be slow and the timer might fall behind.
 * In such a case, the no-drift semantics mean that the timer will attempt to catch up, returning
 * immediately many times until it has caught up with the current time. If this is not intended,
 * the one can do @code if (return_val > time) time = return_val; @endcode to implement a timer
 * that will drift and maintain the same cadence instead of trying to catch back up.
 *
 * @param[in,out] time (nullable) Time in microseconds from an undefined epoch.
 * If \p time is null, the method simply waits for duration_us.
 * The duration is added to the value in \p time, unless the method is interrupted
 * by a signal or an error occurs, in which case time is not updated.
 * If time is null it does not return when interrupted by a signal.
 * @param[in] duration_us Duration after \p time to wait for.
 * If \p duration_us is <= 0, it simply returns the current time.
 * @return The time at which the method was entered.
 */
CLOD_API
int64_t clod_timer(int64_t *time, int64_t duration_us);

/**
 * Simple mutex.
 */
typedef int clod_mutex;
#define CLOD_MUTEX_INIT 0

/** Lock a mutex.
 * It does not support recursive locking.
 * @param[in] mutex Mutex to lock.
 * @return True on success, false on failure.
 * Failure is likely due to lack of resources. */
CLOD_API CLOD_NONNULL(1)
void clod_mutex_lock(clod_mutex *mutex);

/** Unlock a mutex.
 * @param[in] mutex Mutex to unlock. */
CLOD_API CLOD_NONNULL(1)
void clod_mutex_unlock(clod_mutex *mutex);

/** @return true if the timeout was reached, false on normal wakeup. */
CLOD_API CLOD_NONNULL(1)
bool clod_futex_wait(const int *ptr, int expected, int64_t timeout_us);

CLOD_API CLOD_NONNULL(1)
void clod_futex_wake_one(const int *ptr);

CLOD_API CLOD_NONNULL(1)
void clod_futex_wake_all(const int *ptr);

/** @} */
#endif
