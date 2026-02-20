/**
 * @file bigint.h Big Integers
 * @defgroup big_integers Big Integers
 * Dumb and over-engineered arbitrary size integer implementation.
 * Useful as a bit-array implementation too.
 * TODO: support more than simple bit-array functionality here.
 * @{
 */
#ifndef LIBCLOD_BIGINT_H
#define LIBCLOD_BIGINT_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

/// Number of bits in the largest number \p clod_bigint can represent.
#define CLOD_BIGINT_MAX UINT32_MAX

/// Size of a static bigint number with the given number of bits.
/// It is always a multiple of 8.
#define CLOD_BIGINT_STATIC_SIZE(bits) (8 + ((size_t)(bits) + 63) / 64 * 8)

/// Signed big integer representation.
/// Fields should not be modified at risk of breaking internal assurances.
/// Numbers are stored in sign-magnitude format, with sign existing in metadata.
typedef struct clod_bigint clod_bigint;

enum clod_bigint_result {
	/// All good.
	CLOD_BIGINT_OK = 0,
	/// The operation overflowed the size of the bigint because memory could not be allocated.
	CLOD_BIGINT_OVERFLOW = 1
};

/**
 * Initialise a bigint with a dynamic size using an allocation method.
 * The bigint must be freed with \p clod_bigint_free after usage.
 * During usage the bigint will reallocate to grow as needed, and as such
 * any overflow error with a dynamic bigint must be the result of either
 * allocation failure or exhausting CLOD_BIGINT_MAX bits.
 * @code
 * clod_bigint *bigint = clod_bigint_dynamic(nullptr, nullptr);
 * if (!bigint) handle_allocate_failure();
 * // bigint will grow as needed when used.
 * ...
 * clod_bigint_free(bigint);
 * @endcode
 *
 * @param[in] realloc_func (nullable) Method used to allocate memory.
 * If null, defaults to using stdlib realloc.
 * @param[in] user (nullable) Value passed to invocations of \p realloc_func.
 * @return New bigint, or nullptr on allocation failure.
 */
CLOD_API CLOD_USE_RETURN
clod_bigint *clod_bigint_dynamic(
	void*(*realloc_func)(void *ptr, size_t size, void* user),
	void *user
);

/**
 * Free a dynamic-sized bigint using its provided allocation method.
 * If the bigint is static the method does nothing.
 * @param[in] bigint The bigint to free.
 */
CLOD_API CLOD_NONNULL(1)
void clod_bigint_free(clod_bigint *bigint);

/**
 * Initialise a bigint with a static size using user-provided memory.
 * The user is responsible for freeing the memory themselves.
 * @code
 * // Allocate on the stack.
 * uint64_t data[CLOD_BIGINT_STATIC_SIZE(128) / sizeof(uint64_t)];
 * clod_bigint *bigint = clod_bigint_static(data, 128);
 * // Can't fail with valid size and alignment.
 * @endcode
 *
 * @param[in] memory Memory to use.
 * Must be at least \p CLOD_BIGINT_STATIC_SIZE(bits) in size and aligned to uint64_t (8).
 * @param[in] bits Number of bits the bigint can store.
 * @return The same pointer passed in \p memory, or null if \p memory is not aligned to uint64_t (8).
 */
CLOD_API CLOD_NONNULL(1)
clod_bigint *clod_bigint_static(void *memory, uint32_t bits);

/**
 * Get the result of the last operation.
 * Most methods have no failure mode or one failure mode with a clear sentinel return value,
 * so this method is generally not required for normal usage.
 * @param[in] bigint Bigint to get the result for.
 * @return Result of the last operation performed on \p bigint.
 */
CLOD_API CLOD_NONNULL(1)
enum clod_bigint_result clod_bigint_result(const clod_bigint *bigint);

/**
 * Get a bigint's sign. True means the number is negative and false means
 * the number is positive.
 * @param[in] bigint Bigint to get sign of.
 * @return The sign of \p bigint.
 */
CLOD_API CLOD_NONNULL(1)
bool clod_bigint_sign(const clod_bigint *bigint);

/**
 * Set a bigint's sign. True means the number is negative and false means
 * the number is positive. The magnitude of the number is preserved.
 * If the number is 0, the operation is a no-op.
 * @param[in] bigint Bigint to set sign of.
 * @param[in] sign Sign to set \p bigint to.
 */
CLOD_API CLOD_NONNULL(1)
void clod_bigint_set_sign(clod_bigint *bigint, bool sign);

/**
 * Get the number of bits required to represent the number.
 * @param[in] bigint The bigint storing the number.
 * @return Size in bits of the number.
 */
CLOD_API CLOD_NONNULL(1)
uint32_t clod_bigint_bits(const clod_bigint *bigint);

/**
 * Compare two bigints.
 * @param[in] bigint1 First big integer.
 * @param[in] bigint2 Second big integer.
 * @return 1 if \p bigint1 > \p bigint2 ,
 * 0 if \p bigint1 == \p bigint2 , or
 * -1 if \p bigint1 < \p bigint2.
 */
CLOD_API CLOD_NONNULL(1, 2)
int clod_bigint_cmp(const clod_bigint *bigint1, const clod_bigint *bigint2);

/**
 * Set a bigint to the value of a uint64.
 * @param[in] bigint Bigint to assign.
 * @param[in] val Value to assign bigint to.
 * @return 0 on success, or the required size in bits of \p bigint on overflow.
 */
CLOD_API CLOD_NONNULL(1)
uint32_t clod_bigint_set_uint64(clod_bigint *bigint, uint64_t val);

/**
 * Set a bigint to the value of an int64.
 * @param[in] bigint Bigint to assign.
 * @param[in] val Value to assign bigint to.
 * @return 0 on success, or the required size in bits of \p bigint on overflow.
 */
CLOD_API CLOD_NONNULL(1)
uint32_t clod_bigint_set_int64(clod_bigint *bigint, int64_t val);

/**
 * Add a bigint to another bigint.
 * @param[in,out] bigint Bigint to add to.
 * @param[in] val Value to add.
 * @return 0 on success, or an upper bound on the size of the result on overflow.
 */
CLOD_API CLOD_NONNULL(1)
uint32_t clod_bigint_add(clod_bigint *bigint, const clod_bigint *val);

/**
 * Subtract a bigint from another bigint.
 * @param[in,out] bigint Bigint to subtract from.
 * @param[in] val Value to subtract.
 * @return 0 on success, or an upper bound on the size of the result on overflow.
 */
CLOD_API CLOD_NONNULL(1)
uint32_t clod_bigint_sub(clod_bigint *bigint, const clod_bigint *val);

/**
 * Divide a bigint by a uint64.
 * @param[in,out] bigint Bigint to divide.
 * @param[in] val Divisor
 * @return Remainder
 */
CLOD_API CLOD_NONNULL(1)
uint64_t clod_bigint_div_uint64(clod_bigint *bigint, uint64_t val);

/**
 * Get the value of a single bit in the bigint.
 * @param[in] bigint Bigint to get bit from.
 * @param[in] index The bit's index.
 * @return True if the bit is set, false if it is not.
 * If index is larger than the bigint's size, it returns false.
 */
CLOD_API CLOD_NONNULL(1)
bool clod_bigint_bit_get(const clod_bigint *bigint, uint32_t index);

/**
 * Set a single bit in the bigint.
 * @param[in] bigint Bigint to set bit in.
 * @param[in] index Index of the bit.
 * @return 0 on success, or the required size in bits of \p bigint on overflow.
 */
CLOD_API CLOD_NONNULL(1)
uint32_t clod_bigint_bit_set(clod_bigint *bigint, uint32_t index);

/**
 * Unset a single bit in the bigint.
 * The method has no failure mode.
 * @param[in] bigint Bigint to unset bit in.
 * @param[in] index Index of the bit.
 */
CLOD_API CLOD_NONNULL(1)
void clod_bigint_bit_unset(clod_bigint *bigint, uint32_t index);

/**
 * Write a bigint to a string.
 * @param[in] bigint Bigint to write to the string.
 * @param[out] buf Buffer to write to.
 * @param[in] buf_size Size of the buffer.
 * @return Number of characters written to the buffer.
 * If buf_size is returned, it means the output was truncated and no null
 * terminating byte exists.
 */
CLOD_API CLOD_NONNULL(1)
size_t clod_bigint_string(clod_bigint *bigint, char *buf, size_t buf_size, int base);

/** @} */
#endif