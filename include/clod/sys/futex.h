/**
 * @file clod/sys/futex.h
 * @ingroup sys System Methods
 *
 * Methods relating to futexes.
 * For example, clod/thread.h uses this to implement some synchronisation primitives.
 */
#ifndef LIBCLOD_FUTEX_H
#define LIBCLOD_FUTEX_H

#include <clod/lib.h>

enum clod_futex_error {
	/// No worries
	CLOD_FUTEX_OK = 0,
	/// Invalid usage i.e. argument, pointer.
	CLOD_FUTEX_INVALID = 1,
	/// The operation was interrupted by a signal.
	CLOD_FUTEX_INTERRUPT = 2,
};

CLOD_API CLOD_NONNULL(1)
enum clod_futex_error
clod_futex_wait(const int *ptr, int expected, int64_t timeout_us);

CLOD_API CLOD_NONNULL(1)
enum clod_futex_error
clod_futex_wake_one(const int *ptr);

CLOD_API CLOD_NONNULL(1)
enum clod_futex_error
clod_futex_wake_all(const int *ptr);

#endif
