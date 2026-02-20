#include "../../../include/clod/lock.h"
#include "futex.h"
#include "../region_format/atomic.h"
#include <clod/region_format.h>
#include <assert.h>
#include <time.h>

#define READ_LOCKS_MAX 63

static int timespec_diff_ms(const struct timespec from, const struct timespec to) {
	return (int)(
		(to.tv_sec - from.tv_sec) * 1000 +
		(to.tv_nsec / 1000000 - from.tv_nsec / 1000000)
	);
}
static inline struct lock decode(uint32_t val) {
	struct lock lock;
	lock.blocked = val;
	lock.write_lock = val >> 1;
	lock.read_locks = val >> 2;
	lock.generation = val >> 8;
	return lock;
}
static inline uint32_t encode(struct lock lock) {
	uint32_t val = 0;
	val |= (uint32_t)lock.blocked;
	val |= (uint32_t)lock.write_lock << 1;
	val |= (uint32_t)lock.read_locks << 2;
	val |= (uint32_t)lock.generation << 8;
	return val;
}

#ifdef __GNUC__
static inline struct lock load(const void *ptr) {
	assert((uintptr_t)ptr % 4 == 0);
	uint32_t be = __atomic_load_n((uint32_t*)ptr, __ATOMIC_SEQ_CST);
	return decode(beu32_dec((uint8_t*)&be));
}
static inline bool cas(void *ptr, struct lock *lock, struct lock desired) {
	assert((uintptr_t)ptr % 4 == 0);
	uint32_t lock_be, desired_be;
	beu32_enc((uint8_t*)&lock_be, encode(*lock));
	beu32_enc((uint8_t*)&desired_be, encode(desired));
	if (!__atomic_compare_exchange_n(
		(uint32_t*)ptr,
		&lock_be,
		desired_be,
		false,
		__ATOMIC_SEQ_CST,
		__ATOMIC_SEQ_CST
	)) {
		*lock = decode(beu32_dec((uint8_t*)&lock_be));
		return false;
	}
	*lock = desired;
	return true;
}
#else
#error "Atomics for this compiler not implemented"
#endif

static inline bool lock_wait(const void *ptr, struct lock expected, int *timeout) {
	assert((uintptr_t)ptr % 4 == 0);
	assert(*timeout > 0);

	if (*timeout < 0)
		return true;

	struct timespec start;
	if (timespec_get(&start, CLOCK_MONOTONIC) == 0)
		return false;

	uint32_t expected_be;
	beu32_enc((uint8_t*)&expected_be, encode(expected));
	if (!ftx_wait((uint32_t*)ptr, expected_be, *timeout))
		return false;

	struct timespec end;
	if (timespec_get(&end, CLOCK_MONOTONIC) == 0)
		return false;

	*timeout -= timespec_diff_ms(start, end);
	return true;
}
static inline bool lock_wake(const void *ptr) {
	assert((uintptr_t)ptr % 4 == 0);
	return ftx_wake_all((uint32_t*)ptr);
}
static inline bool lock_load_progress(const void *ptr, struct lock *lock) {
	const struct lock new = load(ptr);
	const bool progress =
		lock->generation != new.generation ||
		(lock->write_lock && lock->read_locks != new.read_locks);

	*lock = new;
	return progress;
}

enum lock_result lock_ro_lock(const void *ptr, struct lock *lock_out, int dead_threshold_ms) {
	int timeout = dead_threshold_ms;
	struct lock lock = load(ptr);
	while (lock.blocked || lock.write_lock) {
		if (timeout <= 0)
			return LOCK_DEAD;

		if (!lock_wait(ptr, lock, &timeout))
			return LOCK_OTHER;

		if (lock_load_progress(ptr, &lock))
			timeout = dead_threshold_ms;
	}

	*lock_out = lock;
	return LOCK_OK;
}
enum lock_result lock_ro_unlock(const void *ptr, struct lock lock) {
	assert(lock.write_lock == 0);
	const struct lock current = load(ptr);
	if (current.generation != lock.generation)
		return LOCK_INTERRUPTED;
	return LOCK_OK;
}

enum lock_result lock_rd_lock(void *ptr, struct lock *lock_out, int dead_threshold_ms) {
	struct lock lock = load(ptr);
	struct lock want;
	int timeout = dead_threshold_ms;
	do {
		while (lock.blocked || lock.write_lock || lock.read_locks == READ_LOCKS_MAX) {
			if (timeout <= 0)
				return LOCK_DEAD;

			if (!lock_wait(ptr, lock, &timeout))
				return LOCK_OTHER;

			if (lock_load_progress(ptr, &lock))
				timeout = dead_threshold_ms;
		}

		want = lock;
		want.read_locks++;
	} while (!cas(ptr, &lock, want));

	*lock_out = lock;
	return LOCK_OK;
}
enum lock_result lock_rd_unlock(void *ptr, struct lock lock) {
	assert(lock.read_locks > 0);
	struct lock current = load(ptr);
	struct lock want;
	do {
		if (current.generation != lock.generation || !current.read_locks)
			return LOCK_INTERRUPTED;

		want = current;
		want.read_locks--;
	} while (!cas(ptr, &current, want));

	if (lock.read_locks == 0)
		if (!lock_wake(ptr))
			return LOCK_OTHER;

	return LOCK_OK;
}

enum lock_result lock_wr_lock(void *ptr, struct lock *lock_out, int dead_threshold_ms) {
	struct lock lock = load(ptr);
	struct lock want;
	int timeout = dead_threshold_ms;

	do {
		while (lock.blocked || lock.write_lock) {
			if (timeout <= 0) {
				want.blocked = 0;
				want.write_lock = 1;
				want.read_locks = 0;
				want.generation = lock.generation + 1;
				if (cas(ptr, &lock, want)) {
					*lock_out = lock;
					return LOCK_DEAD_WRITER_ACQUIRED;
				}

				timeout = dead_threshold_ms;
				continue;
			}

			if (!lock_wait(ptr, lock, &timeout))
				return LOCK_OTHER;

			if (lock_load_progress(ptr, &lock))
				timeout = dead_threshold_ms;
		}

		if (lock.read_locks) {
			// We need to wait for remaining readers too.
			// Set the writing flag to block any new readers and
			// wait for them to bugger off.
			const uint32_t generation = lock.generation;
			want = lock;
			want.write_lock = 1;
			if (!cas(ptr, &lock, want))
				continue;

			while (lock.read_locks && lock.generation == generation) {
				if (timeout <= 0) {
					// Assume any remaining readers are dead and reset the reader count.
					want = lock;
					want.read_locks = 0;
					want.generation++;
					if (cas(ptr, &lock, want)) {
						*lock_out = lock;
						return LOCK_OK;
					}

					timeout = dead_threshold_ms;
					continue;
				}

				if (!lock_wait(ptr, lock, &timeout))
					return LOCK_OTHER;

				if (lock_load_progress(ptr, &lock))
					timeout = dead_threshold_ms;
			}

			if (lock.generation != generation) {
				// We lost the race to another writer who through we were dead.
				// Or is wrongly implemented for that matter.
				continue;
			}
		};

		want = lock;
		want.write_lock = 1;
		want.generation++;
	} while (!cas(ptr, &lock, want));

	*lock_out = lock;
	return LOCK_OK;
}
enum lock_result lock_wr_refresh(void *ptr, struct lock *lock) {
	struct lock current = load(ptr);
	struct lock want;
	do {
		if (current.generation != lock->generation)
			return LOCK_INTERRUPTED;

		if (!current.write_lock)
			return LOCK_MISUSE;

		want = current;
		want.generation++;
	} while (!cas(ptr, &current, want));

	lock->generation = current.generation;
	return LOCK_OK;
}
enum lock_result lock_wr_unlock(void *ptr, struct lock lock) {
	struct lock current = load(ptr);
	struct lock want;
	do {
		if (current.generation != lock.generation)
			return LOCK_INTERRUPTED;

		if (!current.write_lock)
			return LOCK_MISUSE;


	}
}

enum lock_result lock_wr_lock_many(void *ptr, bitarray dead_owners_out, size_t count, int dead_threshold_ms);
enum lock_result lock_wr_refresh_many(void *ptr, bitarray mask, size_t count);
enum lock_result lock_wr_unlock_many(void *ptr, size_t count);

/*

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