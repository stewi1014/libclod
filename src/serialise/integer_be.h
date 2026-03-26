#ifndef LIBCLOD_SERIALISE_INTEGER_BE_H
#define LIBCLOD_SERIALISE_INTEGER_BE_H

#include <clod/lib.h>
#include <stdint.h>

/// Maximum 8-bit unsigned value.
#define BEU8_MAX   UINT8_C(0xFF)
/// Maximum 16-bit unsigned value.
#define BEU16_MAX UINT16_C(0xFFFF)
/// Maximum 24-bit unsigned value.
#define BEU24_MAX UINT32_C(0xFFFFFF)
/// Maximum 32-bit unsigned value.
#define BEU32_MAX UINT32_C(0xFFFFFFFF)
/// Maximum 40-bit unsigned value.
#define BEU40_MAX UINT64_C(0xFFFFFFFFFF)
/// Maximum 48-bit unsigned value.
#define BEU48_MAX UINT64_C(0xFFFFFFFFFFFF)
/// Maximum 56-bit unsigned value.
#define BEU56_MAX UINT64_C(0xFFFFFFFFFFFFFF)
/// Maximum 64-bit unsigned value.
#define BEU64_MAX UINT64_C(0xFFFFFFFFFFFFFFFF)

/// Encode an 8-bit unsigned integer into big-endian format.
static inline void beu8_enc (uint8_t ptr[1], const uint8_t  val) { ptr[0] = val; }
/// Encode a 16-bit unsigned integer into big-endian format.
static inline void beu16_enc(uint8_t ptr[2], const uint16_t val) { ptr[0] = (uint8_t)(val >> 8); ptr[1] = (uint8_t)(val); }
/// Encode a 24-bit unsigned integer into big-endian format.
static inline void beu24_enc(uint8_t ptr[3], const uint32_t val) { ptr[0] = (uint8_t)(val >> 16); ptr[1] = (uint8_t)(val >> 8); ptr[2] = (uint8_t)(val); }
/// Encode a 32-bit unsigned integer into big-endian format.
static inline void beu32_enc(uint8_t ptr[4], const uint32_t val) { ptr[0] = (uint8_t)(val >> 24); ptr[1] = (uint8_t)(val >> 16); ptr[2] = (uint8_t)(val >> 8); ptr[3] = (uint8_t)(val); }
/// Encode a 40-bit unsigned integer into big-endian format.
static inline void beu40_enc(uint8_t ptr[5], const uint64_t val) { ptr[0] = (uint8_t)(val >> 32); ptr[1] = (uint8_t)(val >> 24); ptr[2] = (uint8_t)(val >> 16); ptr[3] = (uint8_t)(val >> 8); ptr[4] = (uint8_t)(val); }
/// Encode a 48-bit unsigned integer into big-endian format.
static inline void beu48_enc(uint8_t ptr[6], const uint64_t val) { ptr[0] = (uint8_t)(val >> 40); ptr[1] = (uint8_t)(val >> 32); ptr[2] = (uint8_t)(val >> 24); ptr[3] = (uint8_t)(val >> 16); ptr[4] = (uint8_t)(val >> 8); ptr[5] = (uint8_t)(val); }
/// Encode a 56-bit unsigned integer into big-endian format.
static inline void beu56_enc(uint8_t ptr[7], const uint64_t val) { ptr[0] = (uint8_t)(val >> 48); ptr[1] = (uint8_t)(val >> 40); ptr[2] = (uint8_t)(val >> 32); ptr[3] = (uint8_t)(val >> 24); ptr[4] = (uint8_t)(val >> 16); ptr[5] = (uint8_t)(val >> 8); ptr[6] = (uint8_t)(val); }
/// Encode a 64-bit unsigned integer into big-endian format.
static inline void beu64_enc(uint8_t ptr[8], const uint64_t val) { ptr[0] = (uint8_t)(val >> 56); ptr[1] = (uint8_t)(val >> 48); ptr[2] = (uint8_t)(val >> 40); ptr[3] = (uint8_t)(val >> 32); ptr[4] = (uint8_t)(val >> 24); ptr[5] = (uint8_t)(val >> 16); ptr[6] = (uint8_t)(val >> 8); ptr[7] = (uint8_t)(val); }

/// Decode an 8-bit unsigned integer in big-endian format.
CLOD_PURE static inline uint8_t  beu8_dec (const uint8_t ptr[1]) { return ptr[0]; }
/// Decode a 16-bit unsigned integer in big-endian format.
CLOD_PURE static inline uint16_t beu16_dec(const uint8_t ptr[2]) { return (uint16_t)(ptr[0] << 8  | ptr[1]); }
/// Decode a 24-bit unsigned integer in big-endian format.
CLOD_PURE static inline uint32_t beu24_dec(const uint8_t ptr[3]) { return (uint32_t)ptr[0] << 16 | (uint32_t)ptr[1] << 8  | (uint32_t)ptr[2]; }
/// Decode a 32-bit unsigned integer in big-endian format.
CLOD_PURE static inline uint32_t beu32_dec(const uint8_t ptr[4]) { return (uint32_t)ptr[0] << 24 | (uint32_t)ptr[1] << 16 | (uint32_t)ptr[2] << 8  | (uint32_t)ptr[3]; }
/// Decode a 40-bit unsigned integer in big-endian format.
CLOD_PURE static inline uint64_t beu40_dec(const uint8_t ptr[5]) { return (uint64_t)ptr[0] << 32 | (uint64_t)ptr[1] << 24 | (uint64_t)ptr[2] << 16 | (uint64_t)ptr[3] << 8  | (uint64_t)ptr[4]; }
/// Decode a 48-bit unsigned integer in big-endian format.
CLOD_PURE static inline uint64_t beu48_dec(const uint8_t ptr[6]) { return (uint64_t)ptr[0] << 40 | (uint64_t)ptr[1] << 32 | (uint64_t)ptr[2] << 24 | (uint64_t)ptr[3] << 16 | (uint64_t)ptr[4] << 8  | (uint64_t)ptr[5]; }
/// Decode a 56-bit unsigned integer in big-endian format.
CLOD_PURE static inline uint64_t beu56_dec(const uint8_t ptr[7]) { return (uint64_t)ptr[0] << 48 | (uint64_t)ptr[1] << 40 | (uint64_t)ptr[2] << 32 | (uint64_t)ptr[3] << 24 | (uint64_t)ptr[4] << 16 | (uint64_t)ptr[5] << 8  | (uint64_t)ptr[6]; }
/// Decode a 64-bit unsigned integer in big-endian format.
CLOD_PURE static inline uint64_t beu64_dec(const uint8_t ptr[8]) { return (uint64_t)ptr[0] << 56 | (uint64_t)ptr[1] << 48 | (uint64_t)ptr[2] << 40 | (uint64_t)ptr[3] << 32 | (uint64_t)ptr[4] << 24 | (uint64_t)ptr[5] << 16 | (uint64_t)ptr[6] << 8  | (uint64_t)ptr[7]; }

/// Minimum 8-bit signed value.
#define BEI8_MIN   INT8_C(-0x80)
/// Minimum 16-bit signed value.
#define BEI16_MIN INT16_C(-0x8000)
/// Minimum 24-bit signed value.
#define BEI24_MIN INT32_C(-0x800000)
/// Minimum 32-bit signed value.
#define BEI32_MIN INT32_C(-0x80000000)
/// Minimum 40-bit signed value.
#define BEI40_MIN INT64_C(-0x8000000000)
/// Minimum 48-bit signed value.
#define BEI48_MIN INT64_C(-0x800000000000)
/// Minimum 56-bit signed value.
#define BEI56_MIN INT64_C(-0x80000000000000)
/// Minimum 64-bit signed value.
#define BEI64_MIN INT64_C(-0x8000000000000000)

/// Maximum 8-bit signed value.
#define BEI8_MAX   INT8_C(0x7F)
/// Maximum 16-bit signed value.
#define BEI16_MAX INT16_C(0x7FFF)
/// Maximum 24-bit signed value.
#define BEI24_MAX INT32_C(0x7FFFFF)
/// Maximum 32-bit signed value.
#define BEI32_MAX INT32_C(0x7FFFFFFF)
/// Maximum 40-bit signed value.
#define BEI40_MAX INT64_C(0x7FFFFFFFFF)
/// Maximum 48-bit signed value.
#define BEI48_MAX INT64_C(0x7FFFFFFFFFFF)
/// Maximum 56-bit signed value.
#define BEI56_MAX INT64_C(0x7FFFFFFFFFFFFF)
/// Maximum 64-bit signed value.
#define BEI64_MAX INT64_C(0x7FFFFFFFFFFFFFFF)

/// Encode an 8-bit signed integer into big-endian format.
static inline void bei8_enc (uint8_t ptr[1], const int8_t  val) { beu8_enc (ptr, (uint8_t )(val)); }
/// Encode a 16-bit signed integer into big-endian format.
static inline void bei16_enc(uint8_t ptr[2], const int16_t val) { beu16_enc(ptr, (uint16_t)(val)); }
/// Encode a 24-bit signed integer into big-endian format.
static inline void bei24_enc(uint8_t ptr[3], const int32_t val) { beu24_enc(ptr, (uint32_t)(val > BEI24_MAX ? BEI24_MAX : val < BEI24_MIN ? BEI24_MIN : val)); }
/// Encode a 32-bit signed integer into big-endian format.
static inline void bei32_enc(uint8_t ptr[4], const int32_t val) { beu32_enc(ptr, (uint32_t)(val)); }
/// Encode a 40-bit signed integer into big-endian format.
static inline void bei40_enc(uint8_t ptr[5], const int64_t val) { beu40_enc(ptr, (uint64_t)(val > BEI40_MAX ? BEI40_MAX : val < BEI40_MIN ? BEI40_MIN : val)); }
/// Encode a 48-bit signed integer into big-endian format.
static inline void bei48_enc(uint8_t ptr[6], const int64_t val) { beu48_enc(ptr, (uint64_t)(val > BEI48_MAX ? BEI48_MAX : val < BEI48_MIN ? BEI48_MIN : val)); }
/// Encode a 56-bit signed integer into big-endian format.
static inline void bei56_enc(uint8_t ptr[7], const int64_t val) { beu56_enc(ptr, (uint64_t)(val > BEI56_MAX ? BEI56_MAX : val < BEI56_MIN ? BEI56_MIN : val)); }
/// Encode a 64-bit signed integer into big-endian format.
static inline void bei64_enc(uint8_t ptr[8], const int64_t val) { beu64_enc(ptr, (uint64_t)(val)); }

/// Decode an 8-bit signed integer in big-endian format.
CLOD_PURE static inline int8_t  bei8_dec (const uint8_t ptr[1]) { return (int8_t )(beu8_dec (ptr)); }
/// Decode a 16-bit signed integer in big-endian format.
CLOD_PURE static inline int16_t bei16_dec(const uint8_t ptr[2]) { return (int16_t)(beu16_dec(ptr)); }
/// Decode a 24-bit signed integer in big-endian format.
CLOD_PURE static inline int32_t bei24_dec(const uint8_t ptr[3]) { return (int32_t)(beu24_dec(ptr) << 8) >> 8; }
/// Decode a 32-bit signed integer in big-endian format.
CLOD_PURE static inline int32_t bei32_dec(const uint8_t ptr[4]) { return (int32_t)(beu32_dec(ptr)); }
/// Decode a 40-bit signed integer in big-endian format.
CLOD_PURE static inline int64_t bei40_dec(const uint8_t ptr[5]) { return (int64_t)(beu40_dec(ptr) << 24) >> 24; }
/// Decode a 48-bit signed integer in big-endian format.
CLOD_PURE static inline int64_t bei48_dec(const uint8_t ptr[6]) { return (int64_t)(beu48_dec(ptr) << 16) >> 16; }
/// Decode a 56-bit signed integer in big-endian format.
CLOD_PURE static inline int64_t bei56_dec(const uint8_t ptr[7]) { return (int64_t)(beu56_dec(ptr) << 8) >> 8; }
/// Decode a 64-bit signed integer in big-endian format.
CLOD_PURE static inline int64_t bei64_dec(const uint8_t ptr[8]) { return (int64_t)(beu64_dec(ptr)); }

/// Size of a variable length unsigned integer in big-endian format.
CLOD_CONST static inline uint8_t beuv_size(uint64_t val) {
	uint8_t ret = 1;
	while (val > 0x7F) {
		ret++;
		val >>= 7;
	}
	return ret;
}

/// Encode a variable length unsigned integer in big-endian format.
/// @return True if the buffer was large enough.
static inline bool beuv_enc(uint8_t *ptr, const void *end, const uint64_t val) {
	const uint8_t size = beuv_size(val);
	uint8_t i = 0;
	while (i + 1 < size) {
		if (ptr + i == end) return false;
		ptr[i] = (uint8_t)(val >> ((size - i - 1) * 7) | 0b10000000);
		i++;
	}
	if (ptr + i == end) return false;
	ptr[i] = (uint8_t)(val & 0b01111111);
	return true;
}

/// Decode a variable length unsigned integer in big-endian format.
/// @return True if the buffer was large enough.
static inline bool beuv_dec(const uint8_t *ptr, const void *end, uint64_t *val) {
	uint64_t ret = 0;
	uint8_t i = 0;
	while (i < 9 && ptr + i != end && ptr[i] & 0b10000000) {
		ret <<= 7;
		ret |= (uint64_t)(ptr[i] & 0b01111111);
		i++;
	}
	if (ptr + i == end) return false;
	ret <<= 7;
	ret |= (uint64_t)(ptr[i] & 0b01111111);
	*val = ret;
	return true;
}

/// Size of a variable length signed integer in big-endian format.
CLOD_CONST static inline uint8_t beiv_size(int64_t val) {
	uint8_t ret = 1;
	while (val > 0x3F || val < -0x40) {
		ret++;
		val >>= 7;
	}
	return ret;
}

/// Encode a variable length signed integer in big-endian format.
/// @return True if the buffer was large enough.
static inline bool beiv_enc(uint8_t *ptr, const void *end, const int64_t val) {
	const uint8_t size = beiv_size(val);
	uint8_t i = 0;
	while (i + 1 < size) {
		if (ptr + i == end) return false;
		ptr[i] = (uint8_t)((uint64_t)val >> ((size - i - 1) * 7) | 0b10000000);
		i++;
	}
	if (ptr + i == end) return false;
	ptr[i] = (uint8_t)(val & 0b01111111);
	return true;
}

/// Decode a variable length signed integer in big-endian format.
/// @return True if the buffer was large enough.
static inline bool beiv_dec(const uint8_t *ptr, const void *end, int64_t *val) {
	int64_t ret = 0;
	uint8_t i = 0;
	while (i < 9 && ptr + i != end && ptr[i] & 0b10000000) {
		ret <<= 7;
		ret |= (int64_t)(ptr[i] & 0b01111111);
		i++;
	}
	if (ptr + i == end) return false;
	ret <<= 7;
	ret |= (int64_t)(ptr[i] & 0b01111111);
	if (i < 9 && ptr[0] & 0b01000000) ret |= (int64_t)(UINT64_C(~0) << ((i + 1) * 7));
	*val = ret;
	return true;
}

#endif
