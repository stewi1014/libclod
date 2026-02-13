#ifndef LIBCLOD_BITARRAY_H
#define LIBCLOD_BITARRAY_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define BITARRAY_MAX INT64_MAX

#define bitarray(size) union { uint64_t array[((size) + 63) / 64]; uint8_t (*_size)[(size)]; }
#define bitarray_len(bitarray) (sizeof((bitarray)._size[0]))

#define bitarray_get(bitarray, index) internal_bitarray_get_((bitarray).array, bitarray_len(bitarray), (int64_t)(index))
static inline bool internal_bitarray_get_(const uint64_t *array, const size_t bits, const int64_t index) {
	assert(0 <= index && index < (int64_t)bits);
	return array[index / 64] & (UINT64_C(1) << (index & 63));
}

#define bitarray_set(bitarray, index) internal_bitarray_set_((bitarray).array, bitarray_len(bitarray), (int64_t)(index))
static inline bool internal_bitarray_set_(uint64_t *array, const size_t bits, const int64_t index) {
	assert(0 <= index && index < (int64_t)bits);
	const bool prev = array[index / 64] & (UINT64_C(1) << (index & 63));
	array[index / 64] |= (UINT64_C(1) << (index & 63));
	return prev;
}

#define bitarray_unset(bitarray, index) internal_bitarray_unset_((bitarray).array, bitarray_len(bitarray), (int64_t)(index))
static inline bool internal_bitarray_unset_(uint64_t *array, const size_t bits, const int64_t index) {
	assert(0 <= index && index < (int64_t)bits);
	const bool prev = array[index / 64] & (UINT64_C(1) << (index & 63));
	array[index / 64] &=~ (UINT64_C(1) << (index & 63));
	return prev;
}

#define bitarray_unset_all(bitarray) memset(&(bitarray).array, 0, sizeof((bitarray).array))
#define bitarray_set_all(bitarray) memset(&(bitarray).array, 0xFF, sizeof((bitarray).array))

#define bitarray_any_set(bitarray) internal_bitarray_any_set_((bitarray).array, birarray_len(bitarray))
static inline bool internal_bitarray_any_set_(const uint64_t *array, const size_t bits) {
	for (size_t i = 0; i < bits / 64; i++)
		if (array[i]) return true;
	return bits & 63 ? array[bits / 64] & ((UINT64_C(1) << (bits & 63)) - 1) : false;
}

#define bitarray_all_set(bitarray) internal_bitarray_all_set_((bitarray).array, bitarray_len(bitarray))
static inline bool internal_bitarray_all_set_(uint64_t *array, const size_t bits) {
	for (size_t i = 0; i < bits / 64; i++)
		if (array[i] != ~UINT64_C(0)) return false;
	return bits & 63 ? (array[bits / 64] & ((UINT64_C(1) << (bits & 63)) - 1)) == ((UINT64_C(1) << (bits & 63)) - 1) : true;
}

#endif
