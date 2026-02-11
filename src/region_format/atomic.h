#ifndef LIBCLOD_ATOMIC_H
#define LIBCLOD_ATOMIC_H

#if defined(__GNUC__)

#include "endian_big.h"
#include <assert.h>
#include <stdint.h>

static_assert(__GCC_ATOMIC_INT_LOCK_FREE == 2, "Locking alternatives are not portable across languages.");

static inline uint32_t atomic_beu32_load(const uint8_t *ptr) {
	assert((uintptr_t)ptr % 4 == 0);
	uint32_t val = __atomic_load_n((uint32_t*)ptr, __ATOMIC_SEQ_CST);
	return beu32_dec((uint8_t*)&val);
}

static inline void atomic_beu32_store(uint8_t *ptr, uint32_t val) {
	assert((uintptr_t)ptr % 4 == 0);
	uint32_t be;
	beu32_enc((uint8_t*)&be, val);
	__atomic_store_n((uint32_t*)ptr, be, __ATOMIC_SEQ_CST);
}

static inline bool atomic_beu32_cas(uint8_t *ptr, uint32_t *expected, uint32_t desired) {
	assert((uintptr_t)ptr % 4 == 0);
	uint32_t expected_be, desired_be;
	beu32_enc((uint8_t*)&expected_be, *expected);
	beu32_enc((uint8_t*)&desired_be, desired);
	const bool success = __atomic_compare_exchange_n((uint32_t*)ptr, &expected_be, desired_be, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	if (!success) {
		*expected = beu32_dec((uint8_t*)&expected_be);
	}
	return success;
}

static inline uint32_t atomic_beu32_add(uint8_t *ptr, uint32_t delta) {
	assert((uintptr_t)ptr % 4 == 0);
	uint32_t val = atomic_beu32_load(ptr);
	while (!atomic_beu32_cas(ptr, &val, val + delta)) {}
	return val;
}

static inline uint32_t atomic_beu32_sub(uint8_t *ptr, uint32_t delta) {
	assert((uintptr_t)ptr % 4 == 0);
	uint32_t val = atomic_beu32_load(ptr);
	while (!atomic_beu32_cas(ptr, &val, val - delta)) {}
	return val;
}

#else
#error "Atomic methods not implemented for this compiler"
#endif

#endif
