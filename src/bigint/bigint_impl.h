#ifndef LIBCLOD_BIGINT_IMPL_H
#define LIBCLOD_BIGINT_IMPL_H

#include "block.h"
#include <clod/bigint.h>
#include <stdint.h>

struct clod_bigint {
	/// Length of the bigint in bits.
	uint32_t bits;

	/// Capacity of the bigint in uint64s.
	/// By having this be in units of 64 bits, we can cover the whole
	/// 2^32 bit range with 32 - log2(64) = 26 bits.
	uint32_t cap: 26;

	/// Sign of the number.
	signed sign: 1;

	/// If the bigint has a dynamic or static size.
	/// If it is dynamic, this struct is nested in clod_bigint_dynamic.
	/// If it is static, the array follows this struct.
	bool dynamic: 1;

	/// Result of the last failed operation.
	enum clod_bigint_result result: 2;

	int _unused: 2;
};

struct clod_bigint_static {
	/// Bigint metadata.
	clod_bigint bigint;
	/// Bigint bits.
	block array[];
};

struct clod_bigint_dynamic {
	/// Bigint metadata.
	clod_bigint bigint;
	/// Pointer to the array
	block *array;
	/// Allocation method.
	void*(*realloc_func)(void *ptr, size_t size, void* user);
	/// User-provided value.
	void *user;
};

static_assert(alignof(struct clod_bigint_static) <= 8);
static_assert(sizeof(struct clod_bigint_static) + sizeof(block) * 0 == CLOD_BIGINT_STATIC_SIZE(0));
static_assert(sizeof(struct clod_bigint_static) + sizeof(block) * 1 == CLOD_BIGINT_STATIC_SIZE(1));
static_assert(sizeof(struct clod_bigint_static) + sizeof(block) * 2 == CLOD_BIGINT_STATIC_SIZE(128));
static_assert(sizeof(struct clod_bigint_static) + sizeof(block) * 3 == CLOD_BIGINT_STATIC_SIZE(129));

/// The array of uint64s
#define ARRAY(bigint) ((bigint)->dynamic ?\
	(((struct clod_bigint_dynamic*)(bigint))->array) :\
	(((struct clod_bigint_static*)(bigint))->array)\
)
/// Index in the array of a particular bit.
#define INDEX(bit) ((bit) / BLOCK_BITS)
/// The bit's mask in its array element.
#define MASK(bit) (UINT64_C(1) << ((bit) % BLOCK_BITS))
/// Length of an array in uint64s needed to store the given number of bits.
#define ARRAY_LEN(bits) ((bits) / BLOCK_BITS + ((bits) % BLOCK_BITS ? 1 : 0))
/// The dynamic struct, if the bigint is dynamic.
#define DYNAMIC(bigint) ((bigint)->dynamic ?\
	((struct clod_bigint_dynamic*)(bigint)) :\
	nullptr\
)

static inline uint32_t min(const uint32_t a, const uint32_t b) { return a > b ? b : a; }
static inline uint32_t max(const uint32_t a, const uint32_t b) { return a > b ? a : b; }

/** Subtract the bit count until it fits the existing magnitude.
 * @return If the bigint was already normalised. */
bool normalise(clod_bigint *bigint);

/** Compare the magnitudes of two bigints. */
int magnitude_cmp(const clod_bigint *bigint1, const clod_bigint *bigint2, uint32_t *index);

/** Grow the bigint if possible when it does not have the required capacity.
 * @return Actual capacity after any growth. Can be smaller or larger than cap. */
uint32_t grow(clod_bigint *bigint, uint32_t cap);

/** Came as grow, but clears new space. */
uint32_t grow_clear(clod_bigint *bigint, uint32_t cap);

#endif
