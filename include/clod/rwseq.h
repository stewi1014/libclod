/**
 * @file rwseq.h
 * @defgroup lock Locks
 * @{
 */
#ifndef LIBCLOD_LOCK_H
#define LIBCLOD_LOCK_H

#include <stdint.h>
#include <stddef.h>

enum clod_rwseq_result {
	/// No worries at all!
	CLOD_RWSEQ_OK = 0,
	/// An invalid operation was attempted (e.g. releasing without prior acquisition), and
	/// the detection of the validity of the operation is only incidentally effective.
	/// Put another way, every return of this value is a lucky coincidence that may have
	/// prevented silent data loss, and other cases of the same misuse may not be so lucky.
	/// Notably, the misuse may also be on behalf of a different thread or process.
	CLOD_RWSEQ_MISUSE = 1,
	/// An error which is reasonably assumed to never happen happened.
	/// Examples include the system clock being unavailable, or a critical internal bug
	/// that causes an invalid pointer to be passed to a syscall.
	CLOD_RWSEQ_OTHER = 2,
	/// <b>The lock was acquired from a previous dead write lock holder.</b>
	/// The lock made no progress over a period greater than the dead lock timeout threshold,
	/// and the lock was forcibly acquired from a dead write lock holder. Upon this return value,
	/// the data the lock was protecting may have been left in an inconsistent state by the previous
	/// write lock holder. The method must then either restore the data and cleanly release the lock,
	/// or abort without releasing the lock. It is valid to refresh the lock in this state.
	/// It is only returned by write lock acquisition methods.
	/// Read lock acquisition methods instead return CLOD_RWSEQ_DEAD.
	CLOD_RWSEQ_DEAD_ACQUIRED = 3,
	/// The lock made no progress over a period greater than the dead lock timeout threshold.
	/// Methods are strongly encouraged to attempt to fix the data the lock protects and the lock itself
	/// following this return value. Otherwise, subsequent read lock acquiring methods will also spend the
	/// timeout threshold waiting on the same dead lock - a potential avenue for unacceptable performance.
	/// This would entail acquiring the write lock instead, as fixing the
	CLOD_RWSEQ_DEAD = 4,
	/// <b>The lock was lost at some point before this return value.</b>
	/// The operation was interrupted by an acquisition; Two threads thought they held a lock at the same time.
	/// In normal operation this is only returned by clod_rwseq_ro_unlock, as the read-only lock cannot block
	/// a writer from acquiring a lock. Other methods only return this if they have been kicked out by an
	/// acquirer that thought they were dead. Such a case opens up the possibility for multiple writers to
	/// think they hold a lock at the same time. This <b>needs</b> to be addressed by increasing the dead lock
	/// timeout threshold and/or the cadence with which the slow lock holder refreshes its lock.
	/// It may also be returned if the provided sequence is incorrect.
	CLOD_RWSEQ_INTERRUPTED = 5
};

enum clod_rwseq_result clod_rwseq_ro_lock(const uint32_t *ptr, uint32_t *seq_out);
enum clod_rwseq_result clod_rwseq_ro_unlock(const uint32_t *ptr, uint32_t seq);

enum clod_rwseq_result clod_rwseq_rd_lock(uint32_t *ptr);
enum clod_rwseq_result clod_rwseq_rd_unlock(uint32_t *ptr);

enum clod_rwseq_result clod_rwseq_wr_lock(uint32_t *ptr);
enum clod_rwseq_result clod_rwseq_wr_unlock(uint32_t *ptr);

enum clod_rwseq_result clod_rwseq_wr_lock_many(uint32_t *ptr, size_t count);
enum clod_rwseq_result clod_rwseq_wr_unlock_many(uint32_t *ptr, size_t count);

/** @} */
#endif
