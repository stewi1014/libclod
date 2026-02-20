#ifndef LIBCLOD_BITARRAY_H
#define LIBCLOD_BITARRAY_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define BITARRAY_MAX INT64_MAX

typedef union { struct { uint8_t *array; size_t len; }; uint8_t _len_tag[1][1]; } bitarray;
#define bitarray_array_size(len) (((len) + 7) / 8)
#define bitarray(len_) union { uint8_t array[((len_) + 7) / 8]; size_t len; uint8_t (*_len_tag)[(len_)]; static_assert((len_) <= BITARRAY_MAX); }
#define bitarray_dynamic(const_bita) ((bitarray){ .array = (bita).array, .len = bitarray_len(bita)})
#define bitarray_len(bita) _Generic((bita), bitarray: (size_t)(bita).len, default: sizeof((bita)._len_tag[0]))
#define bitarray_unset_all(bita) memset(&(bita).array, 0, sizeof((bita).array))
#define bitarray_set_all(bita) memset(&(bita).array, 0xFF, sizeof((bita).array))

#define bitarray_get(bita, index) internal_bitarray_get_((bita).array, bitarray_len(bita), (int64_t)(index))
static inline bool internal_bitarray_get_(const uint8_t *array, const size_t len, const int64_t index) {
	assert(0 <= index && index < len);
	return array[index / 8] & (1 << (index & 7));
}

#define bitarray_set(bita, index) internal_bitarray_set_((bita).array, bitarray_len(bita), (int64_t)(index))
static inline bool internal_bitarray_set_(uint8_t *array, const size_t len, const int64_t index) {
	assert(0 <= index && index < len);
	const bool prev = array[index / 8] & (1 << (index % 8));
	array[index / 8] |= (1 << (index % 8));
	return prev;
}

#define bitarray_unset(bita, index) internal_bitarray_unset_((bita).array, bitarray_len(bita), (int64_t)(index))
static inline bool internal_bitarray_unset_(uint8_t *array, const size_t len, const int64_t index) {
	assert(0 <= index && index < len);
	const bool prev = array[index / 8] & (1 << (index % 8));
	array[index / 8] &=~ (1 << (index % 8));
	return prev;
}

#define bitarray_range_eq(bita, start, end, value) ((value)\
	? internal_bitarray_range_set_((bita).array, bitarray_len(bita), (int64_t)(from), (int64_t)(to))\
	: internal_bitarray_range_unset_((bita).array, bitarray_len(bita), (int64_t)(from), (int64_t)(to))\
)

#define bitarray_all_set(bita) internal_bitarray_range_set_((bita).array, bitarray_len(bita), 0, bitarray_len(bita))
static inline bool internal_bitarray_range_set_(const uint8_t *array, const size_t len, const int64_t start, const int64_t end, bool value) {
	if (0 > start || start >= len) return true;
	if (start >= end || end > len) return true;

	int64_t i = start;
	for (;i & 7  && i      < end; i += 1 ) if (!internal_bitarray_get_(array, len, i)) return false;
	for (;i & 63 && i + 8  < end; i += 8 ) if (array[i / 8] != 0xFF) return false;
	for (;          i + 64 < end; i += 64) if (memcmp(array, (uint8_t[8]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8 ) != 0) return false;
	for (;          i + 8  < end; i += 8 ) if (array[i / 8] != 0xFF) return false;
	for (;          i      < end; i += 1 ) if (!internal_bitarray_get_(array, len, i)) return false;
	return true;
}

#define bitarray_all_unset(bita) internal_bitarray_range_unset_((bita).array, bitarray_len(bita), 0, bitarray_len(bita))
static inline bool internal_bitarray_range_unset_(const uint8_t *array, const size_t len, const int64_t start, const int64_t end) {
	if (0 > start || start >= len) return true;
	if (start >= end || end > len) return true;

	int64_t i = start;
	for (;i & 7  && i      < end; i += 1 ) if (internal_bitarray_get_(array, len, i)) return false;
	for (;i & 63 && i + 8  < end; i += 8 ) if (array[i / 8] != 0) return false;
	for (;          i + 64 < end; i += 64) if (memcmp(array, (uint8_t[8]){0, 0, 0, 0, 0, 0, 0, 0}, 8 ) != 0) return false;
	for (;          i + 8  < end; i += 8 ) if (array[i / 8] != 0) return false;
	for (;          i      < end; i += 1 ) if (internal_bitarray_get_(array, len, i)) return false;
	return true;
}

#endif
