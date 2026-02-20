/**
 * @file clod/lock.h
 * @defgroup lock Locks
 * @{
 */
#ifndef LIBCLOD_LOCK_H
#define LIBCLOD_LOCK_H

#include <stdint.h>

struct clod_rwseq {
	uint32_t blocked: 1;
	uint32_t write_lock: 1;
	uint32_t read_locks: 6;
	uint32_t generation: 24;
};

enum clod_rwseq_result {
	/// No worries at all!
	CLOD_RWSEQ_OK,
	/// An invalid operation was attempted (e.g. releasing without prior acquisition), and
	/// the detection of the validity of the operation is only incidentally effective.
	/// Put another way, every return of this value is a lucky coincidence that may have
	/// prevented silent data loss, and other cases of the same misuse may not be so lucky.
	/// Notably, the misuse may also be on behalf of a different thread or process.
	CLOD_RWSEQ_MISUSE,
	/// An error which is reasonably assumed to never happen happened.
	/// Examples include the system clock being unavailable, or a critical internal bug
	/// that causes an invalid pointer to be passed to a syscall.
	CLOD_RWSEQ_OTHER,
	/// <b>The lock is held following this return.</b>
	/// The lock made no progress over a period greater than the dead lock timeout threshold,
	/// and the lock was forcibly acquired from a dead write lock holder. Upon this return value,
	/// the data the lock was protecting may have been left in an inconsistent state by the previous
	/// lock holder. The method must then either restore the data and cleanly release the lock, or
	/// abort without releasing the lock. It is valid to refresh the lock in this state.
	/// It is only returned by write lock acquisition methods.
	/// Read lock acquisition methods instead return CLOD_RWSEQ_DEAD.
	CLOD_RWSEQ_DEAD_WRITER_ACQUIRED,
	/// The lock made no progress over a period greater than the dead lock timeout threshold.
	/// Methods are strongly encouraged to attempt to fix the data the lock protects and the lock itself
	/// following this return value. Otherwise, subsequent read lock acquiring methods will also spend the
	/// timeout threhold waiting on the same dead lock - a potential avenue for anacceptable performance.
	/// This would entail acquiring the write lock instead, as only read lock acquiring methods return this value.
	CLOD_RWSEQ_DEAD,
	/// The operation was interrupted by an acquisition.
	/// In normal operation it is typically returned by methods which cannot exclude the acquisitions
	/// which might interrupt them, but is also returned by methods which can exclude such operations
	/// if someone else kicked us out of the lock.
	CLOD_RWSEQ_INTERRUPTED,
};

enum clod_rwseq_lock_result clod_rwseq_ro_lock(const void *ptr, struct clod_rwseq *lock_out, int dead_threshold_ms);
enum clod_rwseq_lock_result clod_rwseq_ro_unlock(const void *ptr, struct clod_rwseq lock);

enum clod_rwseq_lock_result clod_rwseq_rd_lock(void *ptr, struct clod_rwseq *lock_out, int dead_threshold_ms);
enum clod_rwseq_lock_result clod_rwseq_rd_unlock(void *ptr, struct clod_rwseq lock);

enum clod_rwseq_lock_result clod_rwseq_wr_lock(void *ptr, struct clod_rwseq *lock_out, int dead_threshold_ms);
enum clod_rwseq_lock_result clod_rwseq_wr_refresh(void *ptr, struct clod_rwseq *lock);
enum clod_rwseq_lock_result clod_rwseq_wr_unlock(void *ptr, struct clod_rwseq lock);

enum clod_rwseq_lock_result clod_rwseq_wr_lock_many(void *ptr, uint8_t dead_owners_out, size_t count, int dead_threshold_ms);
enum clod_rwseq_lock_result clod_rwseq_wr_refresh_many(void *ptr, bitarray mask, size_t count);
enum clod_rwseq_lock_result clod_rwseq_wr_unlock_many(void *ptr, bitarray mask, size_t count);

/** @} */

#endif
