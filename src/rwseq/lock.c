#include "config.h"
#include "debug.h"

#include <clod/thread.h>
#include <clod/rwseq.h>
#include <clod/sys/futex.h>
#include <time.h>

#include "keepalive.h"

/// Maximum number of read locks that can be held.
#define READ_LOCKS_MAX 63

/// Even if the real timeout is larger than this,
/// still wake with some kind of cadence in case a different lock user isn't waking us properly.
#define MAX_FUTEX_WAIT_MS 100

/// Timeout before a lock holder is considered dead.
#define DEAD_LOCK_TIMEOUT_MS 10000

struct lock {
	uint32_t blocked:     1;
	uint32_t write_lock:  1;
	uint32_t read_locks:  5;
	uint32_t read_seq:    3;
	uint32_t write_seq:  22;
};
static struct lock decode(uint32_t val) {
	val = (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
	val = (val & 0x00FF00FF) << 8  | (val & 0xFF00FF00) >> 8 ;
	struct lock lock;
	lock.blocked    = ((val >> 0) & 1);
	lock.write_lock = ((val >> 1) & 1);
	lock.read_locks = ((val >>  2) & 31);
	lock.read_seq   = ((val >> 7) & 7);
	lock.write_seq  = (uint64_t)(val >> 10) & 0x3FFFFF;
	return lock;
}
static uint32_t encode(const struct lock lock) {
	uint32_t val =
		(uint32_t)lock.blocked    <<  0|
		(uint32_t)lock.write_lock <<  1|
		(uint32_t)lock.read_locks <<  2|
		(uint32_t)lock.read_seq   <<  7|
		(uint32_t)lock.write_seq  << 10;
	val = (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
	val = (val & 0x00FF00FF) << 8  | (val & 0xFF00FF00) >> 8 ;
	return val;
}
static bool equal(const struct lock lock1, const struct lock lock2) {
	return
		lock1.blocked == lock2.blocked &&
		lock1.write_lock == lock2.write_lock &&
		lock1.read_locks == lock2.read_locks &&
		lock1.read_seq == lock2.read_seq &&
		lock1.write_seq == lock2.write_seq;
}
static struct lock load(const uint32_t *ptr) { return decode(clod_atomic_load(ptr)); }
static bool cas(uint32_t *ptr, struct lock *expected, const struct lock desired) {
	uint32_t expected_val = encode(*expected);
	if (!clod_atomic_cas(ptr, &expected_val, encode(desired))) {
		*expected = decode(expected_val);
		return false;
	}
	*expected = desired;
	return true;
}
static struct timespec now() {
	struct timespec now;
	if (timespec_get(&now, CLOCK_MONOTONIC) == 0) {
		debug(CLOD_RWSEQ_DEBUG, "System clock doesn't support monotonic time.");
		return (struct timespec){0};
	}
	return now;
}
static int time_delta(struct timespec *last) {
	struct timespec current = now();
	int diff =
		(int)((current.tv_sec - last->tv_sec) * 1000) +
		(int)((current.tv_nsec - last->tv_nsec) / 1000000);

	*last = current;
	return diff;
}
static struct lock wait(const uint32_t *ptr, const struct lock expected, int *timeout) {
	auto time = now();
	struct lock lock = load(ptr);

	// One might typically spinwait here a bit, or do some other intermediate waiting before
	// sleeping the thread. Context switches are somewhat expensive, but the intended use case
	// for these locks does not include very brief periods of locking. As such, we assume
	// that it will be a while and go straight to sleep.
	while (equal(lock, expected)) {
		if (*timeout <= 0)
			return lock;

		[[maybe_unused]]
		bool timed_out = clod_futex_wait((int*)ptr, (int)encode(expected), *timeout < MAX_FUTEX_WAIT_MS ? *timeout : MAX_FUTEX_WAIT_MS);

		*timeout -= time_delta(&time);
		lock = load(ptr);

		if (timed_out) {
			// These checks are possibly spurious,
			// as the other thread may have changed the value in the time between us waking up due to timeout
			// and us actually checking the value.
			if (lock.blocked == 0 && expected.blocked)
				debug(CLOD_RWSEQ_DEBUG, "%ptr Blocked flag was unset, but we weren't woken up. Possible bug in other thread.\n", (void*)ptr);
			if (lock.write_lock == 0 && expected.write_lock)
				debug(CLOD_RWSEQ_DEBUG, "%ptr Write lock released, but we weren't woken up. Possible bug in other thread.\n", (void*)ptr);
			if (lock.read_locks == 0 && expected.read_locks)
				debug(CLOD_RWSEQ_DEBUG, "%ptr Read locks released, but we weren't woken up. Possible bug in other thread.\n", (void*)ptr);
		}
	}
	return lock;
}
static void wake(const uint32_t *ptr) { clod_futex_wake_all((int*)ptr); }

enum clod_rwseq_result clod_rwseq_ro_lock(const uint32_t *ptr, uint32_t *seq_out) {
	int timeout = DEAD_LOCK_TIMEOUT_MS;
	struct lock lock = load(ptr);
	while (lock.blocked || lock.write_lock) {
		if (timeout <= 0)
			return CLOD_RWSEQ_DEAD;

		struct lock actual = wait(ptr, lock, &timeout);

		if (actual.write_seq != lock.write_seq)
			timeout = DEAD_LOCK_TIMEOUT_MS;

		lock = actual;
	}

	*seq_out = lock.write_seq;
	return CLOD_RWSEQ_OK;
}
enum clod_rwseq_result clod_rwseq_ro_unlock(const uint32_t *ptr, const uint32_t seq) {
	const struct lock current = load(ptr);

	if (current.write_seq != seq)
		return CLOD_RWSEQ_INTERRUPTED;

	return CLOD_RWSEQ_OK;
}

bool clod_rwseq_rd_heartbeat(void *ptr) {
	struct lock lock = load(ptr);
	struct lock want;
	do {
		if (lock.read_locks == 0) {
			debug(CLOD_RWSEQ_DEBUG, "%ptr Possible failure to unlock read lock. Read sequence heartbeat task left running on lock with no read locks.", ptr);
			return true;
		}

		want = lock;
		want.read_seq++;
	} while (!cas(ptr, &lock, want));
	return false;
}
enum clod_rwseq_result clod_rwseq_rd_lock(uint32_t *ptr) {
	int timeout = DEAD_LOCK_TIMEOUT_MS;
	struct lock lock = load(ptr);
	struct lock want;
	do {
		while (lock.blocked || lock.write_lock || lock.read_locks == READ_LOCKS_MAX) {
			if (timeout <= 0)
				return CLOD_RWSEQ_DEAD;

			struct lock actual = wait(ptr, lock, &timeout);

			if (actual.write_seq != lock.write_seq)
				timeout = DEAD_LOCK_TIMEOUT_MS;
		}

		want = lock;
		want.read_locks++;
	} while (!cas(ptr, &lock, want));

	clod_rwseq_rd_keepalive_start((int*)ptr);
	return CLOD_RWSEQ_OK;
}
enum clod_rwseq_result clod_rwseq_rd_unlock(uint32_t *ptr) {
	clod_rwseq_rd_keepalive_end((int*)ptr);
	struct lock lock = load(ptr);
	struct lock want;
	do {
		if (lock.read_locks == 0) {
			debug(CLOD_RWSEQ_DEBUG, "%ptr Attempted to unlock already unlocked read lock.", (void*)ptr);
			return CLOD_RWSEQ_MISUSE;
		}

		want = lock;
		want.read_locks--;
	} while (!cas(ptr, &lock, want));

	if (lock.read_locks == 0)
		wake(ptr);

	return CLOD_RWSEQ_OK;
}



/*

enum clod_rwseq_result clod_rwseq_rd_lock(uint32_t *ptr) {
	int timeout = DEAD_LOCK_TIMEOUT_MS;
	struct lock lock = load(ptr);

	while (lock.blocked || lock.write_lock || lock.read_locks == READ_LOCKS_MAX) {

	}



	struct lock lock = {0};

	struct lock want;
	int timeout = dead_threshold_ms;
	do {
		while (lock.blocked || lock.write_lock || lock.read_locks == READ_LOCKS_MAX) {
			if (timeout <= 0)
				return CLOD_RWSEQ_DEAD;

			if (!wait(ptr, lock, &timeout))
				return CLOD_RWSEQ_OTHER;

			if (load_progress(ptr, &lock))
				timeout = dead_threshold_ms;
		}

		want = lock;
		want.read_locks++;
	} while (!cas(ptr, &lock, want));

	*seq_out = lock.sequence;
	return CLOD_RWSEQ_OK;
}
enum clod_rwseq_result clod_rwseq_rd_unlock(uint32_t *ptr, uint32_t seq) {
	struct lock lock = load(ptr);
	struct lock want;
	do {
		want = lock;
		if (want.read_locks > 0)
			want.read_locks--;
	} while (!cas(ptr, &lock, want));



	if (lock.read_locks == 0)
		if (!wake(ptr))
			return CLOD_RWSEQ_OTHER;
	return CLOD_RWSEQ_OK;
}

enum clod_rwseq_result clod_rwseq_wr_lock(uint32_t *ptr, int dead_threshold_ms) {
	struct lock lock = load(ptr);
	struct lock want;
	int timeout = dead_threshold_ms;
	bool dead_acquired = false;

	// Acquire the write lock.
	do {
		while (lock.blocked || lock.write_lock) {
			if (timeout <= 0) {
				dead_acquired = true;
				break;
			}

			if (!wait(ptr, lock, &timeout))
				return CLOD_RWSEQ_OTHER;

			if (load_progress(ptr, &lock))
				timeout = dead_threshold_ms;
		}

		want = lock;
		want.blocked = 0;
		want.write_lock = 1;
		want.sequence++;
	} while (!cas(ptr, &lock, want));

	timeout = dead_threshold_ms;
	while (lock.read_locks) {
		// We need to wait for remaining readers to finish.
		if (timeout <= 0) {
			// Assume remaining readers are dead.
			do {
				want = lock;
				want.read_locks = 0;
				want.sequence++;
			} while (!cas(ptr, &lock, want));
		}

		if (!wait(ptr, lock, &timeout))
			return CLOD_RWSEQ_OTHER;

		// Keep our lock alive while we wait for readers.
		do {
			want = lock;
			want.sequence++;
		} while (!cas(ptr, &lock, want));
	}

	return CLOD_RWSEQ_OK;
}
enum clod_rwseq_result clod_rwseq_wr_refresh(uint32_t *ptr) {
	struct lock lock = load(ptr);
	if (!lock.write_lock)
		return CLOD_RWSEQ_INTERRUPTED;

	struct lock want;



	struct lock current = load(ptr);
	struct lock want;
	do {
		if (current.sequence != lock->sequence)
			return LOCK_INTERRUPTED;

		if (!current.write_lock)
			return LOCK_MISUSE;

		want = current;
		want.sequence++;
	} while (!cas(ptr, &current, want));

	lock->sequence = current.sequence;
	return LOCK_OK;
}
enum lock_result lock_wr_unlock(void *ptr, struct lock lock) {
	struct lock current = load(ptr);
	struct lock want;
	do {
		if (current.sequence != lock.sequence)
			return LOCK_INTERRUPTED;

		if (!current.write_lock)
			return LOCK_MISUSE;


	}
}

enum lock_result lock_wr_lock_many(void *ptr, bitarray dead_owners_out, size_t count, int dead_threshold_ms);
enum lock_result lock_wr_refresh_many(void *ptr, bitarray mask, size_t count);
enum lock_result lock_wr_unlock_many(void *ptr, size_t count);

enum lock_result lock_ro_lock(const uint8_t *ptr, struct lock *lock_out, int dead_threshold_ms) {
	struct lock lock = load(ptr);
	int timeout = dead_threshold_ms;

	while (lock.blocked || lock.write_lock) {
		const enum lock_result res = lock_wait(ptr, lock, &timeout);
		if (res != LOCK_OK && res != LOCK_TIMEOUT) return res;

		const struct lock new = load(ptr);
		if (new.write_lock != lock.write_lock || new.generation != lock.generation)
			timeout = dead_threshold_ms;
		else if (res == LOCK_TIMEOUT)
			return res;

		lock = new;
	}

	*lock_out = lock;
	return LOCK_OK;
}
enum lock_result lock_ro_unlock(const uint8_t *ptr, struct lock lock) {
	const struct lock current = load(ptr);
	if (current.generation != lock.generation) {
		return LOCK_INTERRUPTED;
	}
	return LOCK_OK;
}

enum lock_result lock_rd_lock(uint8_t *ptr, struct lock *lock_out, int dead_threshold_ms) {
	struct lock lock = load(ptr);
	struct lock want;
	int timeout = dead_threshold_ms;
	do {
		while (lock.blocked || lock.write_lock || lock.read_locks == READ_LOCKS_MAX) {
			const enum lock_result res = lock_wait(ptr, lock, &timeout);
			if (res != LOCK_OK) return res;

			want = load(ptr);
			if (want.)

			lock = want;
		}

		want = lock;
		want.read_locks++;
	} while (!cas(ptr, &lock, want));
	*lock_out = want;
	return LOCK_OK;
}
*/