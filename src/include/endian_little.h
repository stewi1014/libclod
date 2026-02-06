/**
 * @file endian_little.h
 * @brief Methods for little-endian encoding numbers.
 */

#ifndef LIBCLOD_LITTLE_ENDIAN_H
#define LIBCLOD_LITTLE_ENDIAN_H

#include <clod/lib.h>
#include <limits.h>
#include <stdint.h>

static_assert(__STDC_IEC_60559_BFP__, "bit representation of floats must be ICE 559");
static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);

/// Maximum 8-bit unsigned value.
#define LEU8_MAX   UINT8_C(0xFF)
/// Maximum 16-bit unsigned value.
#define LEU16_MAX UINT16_C(0xFFFF)
/// Maximum 24-bit unsigned value.
#define LEU24_MAX UINT32_C(0xFFFFFF)
/// Maximum 32-bit unsigned value.
#define LEU32_MAX UINT32_C(0xFFFFFFFF)
/// Maximum 40-bit unsigned value.
#define LEU40_MAX UINT64_C(0xFFFFFFFFFF)
/// Maximum 48-bit unsigned value.
#define LEU48_MAX UINT64_C(0xFFFFFFFFFFFF)
/// Maximum 56-bit unsigned value.
#define LEU56_MAX UINT64_C(0xFFFFFFFFFFFFFF)
/// Maximum 64-bit unsigned value.
#define LEU64_MAX UINT64_C(0xFFFFFFFFFFFFFFFF)

/// Encode an 8-bit unsigned integer into little-endian format.
static inline void leu8_enc (uint8_t ptr[1], const uint8_t  val) { ptr[0] = val; }
/// Encode a 16-bit unsigned integer into little-endian format.
static inline void leu16_enc(uint8_t ptr[2], const uint16_t val) { ptr[1] = val >> 8; ptr[0] = val; }
/// Encode a 24-bit unsigned integer into little-endian format.
static inline void leu24_enc(uint8_t ptr[3], const uint32_t val) { ptr[2] = val >> 16; ptr[1] = val >> 8; ptr[0] = val; }
/// Encode a 32-bit unsigned integer into little-endian format.
static inline void leu32_enc(uint8_t ptr[4], const uint32_t val) { ptr[3] = val >> 24; ptr[2] = val >> 16; ptr[1] = val >> 8; ptr[0] = val; }
/// Encode a 40-bit unsigned integer into little-endian format.
static inline void leu40_enc(uint8_t ptr[5], const uint64_t val) { ptr[4] = val >> 32; ptr[3] = val >> 24; ptr[2] = val >> 16; ptr[1] = val >> 8; ptr[0] = val; }
/// Encode a 48-bit unsigned integer into little-endian format.
static inline void leu48_enc(uint8_t ptr[6], const uint64_t val) { ptr[5] = val >> 40; ptr[4] = val >> 32; ptr[3] = val >> 24; ptr[2] = val >> 16; ptr[1] = val >> 8; ptr[0] = val; }
/// Encode a 56-bit unsigned integer into little-endian format.
static inline void leu56_enc(uint8_t ptr[7], const uint64_t val) { ptr[6] = val >> 48; ptr[5] = val >> 40; ptr[4] = val >> 32; ptr[3] = val >> 24; ptr[2] = val >> 16; ptr[1] = val >> 8; ptr[0] = val; }
/// Encode a 64-bit unsigned integer into little-endian format.
static inline void leu64_enc(uint8_t ptr[8], const uint64_t val) { ptr[7] = val >> 56; ptr[6] = val >> 48; ptr[5] = val >> 40; ptr[4] = val >> 32; ptr[3] = val >> 24; ptr[2] = val >> 16; ptr[1] = val >> 8; ptr[0] = val; }

/// Decode an 8-bit unsigned integer in little-endian format.
CLOD_PURE static inline uint8_t  leu8_dec (const uint8_t ptr[1]) { return ptr[0]; }
/// Decode a 16-bit unsigned integer in little-endian format.
CLOD_PURE static inline uint16_t leu16_dec(const uint8_t ptr[2]) { return (uint16_t)ptr[1] << 8  | (uint16_t)ptr[0]; }
/// Decode a 24-bit unsigned integer in little-endian format.
CLOD_PURE static inline uint32_t leu24_dec(const uint8_t ptr[3]) { return (uint32_t)ptr[2] << 16 | (uint32_t)ptr[1] << 8  | (uint32_t)ptr[0]; }
/// Decode a 32-bit unsigned integer in little-endian format.
CLOD_PURE static inline uint32_t leu32_dec(const uint8_t ptr[4]) { return (uint32_t)ptr[3] << 24 | (uint32_t)ptr[2] << 16 | (uint32_t)ptr[1] << 8  | (uint32_t)ptr[0]; }
/// Decode a 40-bit unsigned integer in little-endian format.
CLOD_PURE static inline uint64_t leu40_dec(const uint8_t ptr[5]) { return (uint64_t)ptr[4] << 32 | (uint64_t)ptr[3] << 24 | (uint64_t)ptr[2] << 16 | (uint64_t)ptr[1] << 8  | (uint64_t)ptr[0]; }
/// Decode a 48-bit unsigned integer in little-endian format.
CLOD_PURE static inline uint64_t leu48_dec(const uint8_t ptr[6]) { return (uint64_t)ptr[5] << 40 | (uint64_t)ptr[4] << 32 | (uint64_t)ptr[3] << 24 | (uint64_t)ptr[2] << 16 | (uint64_t)ptr[1] << 8  | (uint64_t)ptr[0]; }
/// Decode a 56-bit unsigned integer in little-endian format.
CLOD_PURE static inline uint64_t leu56_dec(const uint8_t ptr[7]) { return (uint64_t)ptr[6] << 48 | (uint64_t)ptr[5] << 40 | (uint64_t)ptr[4] << 32 | (uint64_t)ptr[3] << 24 | (uint64_t)ptr[2] << 16 | (uint64_t)ptr[1] << 8  | (uint64_t)ptr[0]; }
/// Decode a 64-bit unsigned integer in little-endian format.
CLOD_PURE static inline uint64_t leu64_dec(const uint8_t ptr[8]) { return (uint64_t)ptr[7] << 56 | (uint64_t)ptr[6] << 48 | (uint64_t)ptr[5] << 40 | (uint64_t)ptr[4] << 32 | (uint64_t)ptr[3] << 24 | (uint64_t)ptr[2] << 16 | (uint64_t)ptr[1] << 8  | (uint64_t)ptr[0]; }

/// Minimum 8-bit signed value.
#define LEI8_MIN   INT8_C(-0x80)
/// Minimum 16-bit signed value.
#define LEI16_MIN INT16_C(-0x8000)
/// Minimum 24-bit signed value.
#define LEI24_MIN INT32_C(-0x800000)
/// Minimum 32-bit signed value.
#define LEI32_MIN INT32_C(-0x80000000)
/// Minimum 40-bit signed value.
#define LEI40_MIN INT64_C(-0x8000000000)
/// Minimum 48-bit signed value.
#define LEI48_MIN INT64_C(-0x800000000000)
/// Minimum 56-bit signed value.
#define LEI56_MIN INT64_C(-0x80000000000000)
/// Minimum 64-bit signed value.
#define LEI64_MIN INT64_C(-0x8000000000000000)

/// Maximum 8-bit signed value.
#define LEI8_MAX   INT8_C(0x7F)
/// Maximum 16-bit signed value.
#define LEI16_MAX INT16_C(0x7FFF)
/// Maximum 24-bit signed value.
#define LEI24_MAX INT32_C(0x7FFFFF)
/// Maximum 32-bit signed value.
#define LEI32_MAX INT32_C(0x7FFFFFFF)
/// Maximum 40-bit signed value.
#define LEI40_MAX INT64_C(0x7FFFFFFFFF)
/// Maximum 48-bit signed value.
#define LEI48_MAX INT64_C(0x7FFFFFFFFFFF)
/// Maximum 56-bit signed value.
#define LEI56_MAX INT64_C(0x7FFFFFFFFFFFFF)
/// Maximum 64-bit signed value.
#define LEI64_MAX INT64_C(0x7FFFFFFFFFFFFFFF)

/// Encode an 8-bit signed integer into little-endian format.
static inline void lei8_enc (uint8_t ptr[1], const int8_t  val) { leu8_enc (ptr, (uint8_t )(val)); }
/// Encode a 16-bit signed integer into little-endian format.
static inline void lei16_enc(uint8_t ptr[2], const int16_t val) { leu16_enc(ptr, (uint16_t)(val)); }
/// Encode a 24-bit signed integer into little-endian format.
static inline void lei24_enc(uint8_t ptr[3], const int32_t val) { leu24_enc(ptr, (uint32_t)(val > LEI24_MAX ? LEI24_MAX : val < LEI24_MIN ? LEI24_MIN : val)); }
/// Encode a 32-bit signed integer into little-endian format.
static inline void lei32_enc(uint8_t ptr[4], const int32_t val) { leu32_enc(ptr, (uint32_t)(val)); }
/// Encode a 40-bit signed integer into little-endian format.
static inline void lei40_enc(uint8_t ptr[5], const int64_t val) { leu40_enc(ptr, (uint64_t)(val > LEI40_MAX ? LEI40_MAX : val < LEI40_MIN ? LEI40_MIN : val)); }
/// Encode a 48-bit signed integer into little-endian format.
static inline void lei48_enc(uint8_t ptr[6], const int64_t val) { leu48_enc(ptr, (uint64_t)(val > LEI48_MAX ? LEI48_MAX : val < LEI48_MIN ? LEI48_MIN : val)); }
/// Encode a 56-bit signed integer into little-endian format.
static inline void lei56_enc(uint8_t ptr[7], const int64_t val) { leu56_enc(ptr, (uint64_t)(val > LEI56_MAX ? LEI56_MAX : val < LEI56_MIN ? LEI56_MIN : val)); }
/// Encode a 64-bit signed integer into little-endian format.
static inline void lei64_enc(uint8_t ptr[8], const int64_t val) { leu64_enc(ptr, (uint64_t)(val)); }

/// Decode an 8-bit signed integer in little-endian format.
CLOD_PURE static inline int8_t  lei8_dec (const uint8_t ptr[1]) { return (int8_t )(leu8_dec (ptr)); }
/// Decode a 16-bit signed integer in little-endian format.
CLOD_PURE static inline int16_t lei16_dec(const uint8_t ptr[2]) { return (int16_t)(leu16_dec(ptr)); }
/// Decode a 24-bit signed integer in little-endian format.
CLOD_PURE static inline int32_t lei24_dec(const uint8_t ptr[3]) { return (int32_t)(leu24_dec(ptr) << 8) >> 8; }
/// Decode a 32-bit signed integer in little-endian format.
CLOD_PURE static inline int32_t lei32_dec(const uint8_t ptr[4]) { return (int32_t)(leu32_dec(ptr)); }
/// Decode a 40-bit signed integer in little-endian format.
CLOD_PURE static inline int64_t lei40_dec(const uint8_t ptr[5]) { return (int64_t)(leu40_dec(ptr) << 24) >> 24; }
/// Decode a 48-bit signed integer in little-endian format.
CLOD_PURE static inline int64_t lei48_dec(const uint8_t ptr[6]) { return (int64_t)(leu48_dec(ptr) << 16) >> 16; }
/// Decode a 56-bit signed integer in little-endian format.
CLOD_PURE static inline int64_t lei56_dec(const uint8_t ptr[7]) { return (int64_t)(leu56_dec(ptr) << 8) >> 8; }
/// Decode a 64-bit signed integer in little-endian format.
CLOD_PURE static inline int64_t lei64_dec(const uint8_t ptr[8]) { return (int64_t)(leu64_dec(ptr)); }

/// Encode a float in little-endian format.
static inline void lef32_enc(uint8_t ptr[4], const float  f) { const union { float  f; uint32_t i; } u = { f }; leu32_enc(ptr, u.i); }
/// Encode a double in little-endian format.
static inline void lef64_enc(uint8_t ptr[8], const double f) { const union { double f; uint64_t i; } u = { f }; leu64_enc(ptr, u.i); }

/// Decode a float in little_endian format.
CLOD_PURE static inline float  lef32_dec(const uint8_t ptr[4]) { const union { float  f; uint32_t i; } u = { .i = leu32_dec(ptr) }; return u.f; }
/// Decode a double in little_endian format.
CLOD_PURE static inline double lef64_dec(const uint8_t ptr[8]) { const union { double f; uint64_t i; } u = { .i = leu64_dec(ptr) }; return u.f; }

/// Maximum varint unsigned value.
#define LEUV_MAX UINT64_MAX

/// Size of a variable length unsigned integer in little-endian format.
CLOD_CONST static inline uint8_t leuv_size(uint64_t val) {
	uint8_t ret = 1;
	while (val > 0x7F) {
		ret++;
		val >>= 7;
	}
	return ret;
}

/// Encode a variable length unsigned integer in little-endian format.
/// @return True if the buffer was large enough.
static inline bool leuv_enc(uint8_t *ptr, const void *end, uint64_t val) {
	const uint8_t size = leuv_size(val);
	uint8_t i = 0;
	while (i + 1 < size) {
		if (ptr + i == end) return false;
		ptr[i] = (uint8_t)(val >> (i * 7) | 0b10000000);
		i++;
	}
	if (ptr + i == end) return false;
	ptr[i] = (uint8_t)(val >> (i * 7) & 0b01111111);
	return true;
}

/// Decode a variable length unsigned integer in little-endian format.
/// @return True if the buffer was large enough.
static inline bool leuv_dec(const uint8_t *ptr, const void *end, uint64_t *val) {
	uint64_t ret = 0;
	uint8_t i = 0;
	while (i < 9 && ptr + i != end && ptr[i] & 0b10000000) {
		ret |= (uint64_t)(ptr[i] & 0b01111111) << (i * 7);
		i++;
	}
	if (ptr + i == end) return false;
	ret |= (uint64_t)(ptr[i] & 0b01111111) << (i * 7);
	*val = ret;
	return true;
}

/// Maximum variable length signed value.
#define LEIV_MAX INT64_MAX
/// Minimum variable length signed value.
#define LEIV_MIN INT64_MIN

/// Size of a variable length signed integer in little-endian format.
CLOD_CONST static inline uint8_t leiv_size(int64_t val) {
	uint8_t ret = 1;
	while (val > 0x3F || val < -0x40) {
		ret++;
		val >>= 7;
	}
	return ret;
}

/// Encode a variable length signed integer in little-endian format.
/// @return True if the buffer was large enough.
static inline bool leiv_enc(uint8_t *ptr, const void *end, int64_t val) {
	const uint8_t size = leiv_size(val);
	uint8_t i = 0;
	while (i + 1 < size) {
		if (ptr + i == end) return false;
		ptr[i] = (uint8_t)(val >> (i * 7) | 0b10000000);
		i++;
	}
	if (ptr + i == end) return false;
	ptr[i] = (uint8_t)((uint64_t)val >> (i * 7) & 0b01111111);
	return true;
}

/// Decode a variable length signed integer in little-endian format.
/// @return True if the buffer was large enough.
static inline bool leiv_dec(const uint8_t *ptr, const void *end, int64_t *val) {
	int64_t ret = 0;
	uint8_t i = 0;
	while (i < 9 && ptr + i != end && ptr[i] & 0b10000000) {
		ret |= (int64_t)(ptr[i] & 0b01111111) << (i * 7);
		i++;
	}
	if (ptr + i == end) return false;
	ret |= (int64_t)(uint64_t)(ptr[i] & 0b01111111) << (i * 7);
	if (i < 9 && ptr[i] & 0b01000000) ret |= INT64_C(-1) << ((i + 1) * 7);
	*val = ret;
	return true;
}

#endif
