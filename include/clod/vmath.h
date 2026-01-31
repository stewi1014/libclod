/**
 * @file vmath.h
 * @brief Math and vector methods.
 *
 */

#ifndef LIBCLOD_VMATH_H
#define LIBCLOD_VMATH_H

#include <clod/lib.h>
#include <stdint.h>

static_assert(__STDC_VERSION__ >= 202311L, "Some behavior only defined in C23 is depended upon");

CLOD_CONST CLOD_INLINE
static inline uint64_t mask64(uint8_t bits) {
	if (bits >= 64) return UINT64_MAX;
	return (UINT64_C(1) << bits) - 1;
}

/** division the way god intended */
CLOD_CONST CLOD_INLINE
static inline struct divi64 { int64_t quot; int64_t rem; }
divi64(const int64_t x, const int64_t divisor) {
	struct divi64 res;
	res.quot = x / divisor;
	res.rem = x % divisor;

	if (res.rem != 0 && res.rem < 0 != divisor < 0) {
		res.rem += divisor;
		res.quot--;
	}

	return res;
}

/**
 * Group points together into groups of size 2^\p group_bits,
 * modifying the vector to point to the new group.
 *
 * If \p group_bits is divisible by \p vec_len, it's really quite simple.
 * For example,
 * 10 bits per group, 2 dimensions = 5 bits from each dimension,
 * each group is 2^5 * 2^5.
 * 10 bits per group, 5 dimensions = 2 bits from each dimension,
 * each group is 2^2 * 2^2 * 2^2 * 2^2 * 2^2.
 *
 * The real reason for this method beyond simple division is when \p group_bits is *not* divisible by \p vec_len.
 * For example, for 3 dimensions and 10 bits it does 2^4 * 2^3 * 2^3.
 *
 * @param vec
 * @param vec_len
 * @param group_bits
 * @return
 */
CLOD_INLINE CLOD_NONNULL(1)
static inline uint64_t vec_group(int64_t *restrict vec, uint8_t vec_len, uint8_t group_bits) {
	if (group_bits > 64) group_bits = 64;
	uint64_t res = 0;
	uint8_t res_bits = 0;
	for (uint8_t i = 0; i < vec_len; i++) {
		const uint8_t take_bits = (group_bits - res_bits + vec_len - i - 1) / (vec_len - i);
		res |= ((uint64_t)vec[i] & mask64(take_bits)) << res_bits;
		vec[i] >>= take_bits;
		res_bits += take_bits;
	}
	return res;
}

#endif //LIBCLOD_VMATH_H