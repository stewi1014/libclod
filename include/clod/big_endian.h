/**
 * @file big_endian.h
 * @brief Methods for big-endian encoding numbers.
 */

#ifndef CLOD_BIG_ENDIAN_H
#define CLOD_BIG_ENDIAN_H

#include <clod/lib.h>
#include <limits.h>
#include <stdint.h>

static_assert(CHAR_BIT == 8);
static_assert(__STDC_IEC_60559_BFP__, "bit representation of floats must be ICE 559");
static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);

#define BEU8_MAX   UINT8_C(0xFF)
#define BEU16_MAX UINT16_C(0xFFFF)
#define BEU24_MAX UINT32_C(0xFFFFFF)
#define BEU32_MAX UINT32_C(0xFFFFFFFF)
#define BEU40_MAX UINT64_C(0xFFFFFFFFFF)
#define BEU48_MAX UINT64_C(0xFFFFFFFFFFFF)
#define BEU56_MAX UINT64_C(0xFFFFFFFFFFFFFF)
#define BEU64_MAX UINT64_C(0xFFFFFFFFFFFFFFFF)

CLOD_INLINE static inline void beu8_enc (char ptr[1], const uint8_t  val) { ptr[0] = (char)(val); }
CLOD_INLINE static inline void beu16_enc(char ptr[2], const uint16_t val) { ptr[0] = (char)(val >> 8 ); ptr[1] = (char)(val); }
CLOD_INLINE static inline void beu24_enc(char ptr[3], const uint32_t val) { ptr[0] = (char)(val >> 16); ptr[1] = (char)(val >> 8 ); ptr[2] = (char)(val); }
CLOD_INLINE static inline void beu32_enc(char ptr[4], const uint32_t val) { ptr[0] = (char)(val >> 24); ptr[1] = (char)(val >> 16); ptr[2] = (char)(val >> 8 ); ptr[3] = (char)(val); }
CLOD_INLINE static inline void beu40_enc(char ptr[5], const uint64_t val) { ptr[0] = (char)(val >> 32); ptr[1] = (char)(val >> 24); ptr[2] = (char)(val >> 16); ptr[3] = (char)(val >> 8 ); ptr[4] = (char)(val); }
CLOD_INLINE static inline void beu48_enc(char ptr[6], const uint64_t val) { ptr[0] = (char)(val >> 40); ptr[1] = (char)(val >> 32); ptr[2] = (char)(val >> 24); ptr[3] = (char)(val >> 16); ptr[4] = (char)(val >> 8 ); ptr[5] = (char)(val); }
CLOD_INLINE static inline void beu56_enc(char ptr[7], const uint64_t val) { ptr[0] = (char)(val >> 48); ptr[1] = (char)(val >> 40); ptr[2] = (char)(val >> 32); ptr[3] = (char)(val >> 24); ptr[4] = (char)(val >> 16); ptr[5] = (char)(val >> 8 ); ptr[6] = (char)(val); }
CLOD_INLINE static inline void beu64_enc(char ptr[8], const uint64_t val) { ptr[0] = (char)(val >> 56); ptr[1] = (char)(val >> 48); ptr[2] = (char)(val >> 40); ptr[3] = (char)(val >> 32); ptr[4] = (char)(val >> 24); ptr[5] = (char)(val >> 16); ptr[6] = (char)(val >> 8 ); ptr[7] = (char)(val); }

CLOD_INLINE static inline uint8_t  beu8_dec (const char ptr[1]) { return (uint8_t)ptr[0]; }
CLOD_INLINE static inline uint16_t beu16_dec(const char ptr[2]) { return (uint16_t)((uint16_t)(uint8_t)ptr[0] << 8  | (uint16_t)(uint8_t)ptr[1]); }
CLOD_INLINE static inline uint32_t beu24_dec(const char ptr[3]) { return (uint32_t)((uint32_t)(uint8_t)ptr[0] << 16 | (uint32_t)(uint8_t)ptr[1] << 8  | (uint32_t)(uint8_t)ptr[2]); }
CLOD_INLINE static inline uint32_t beu32_dec(const char ptr[4]) { return (uint32_t)((uint32_t)(uint8_t)ptr[0] << 24 | (uint32_t)(uint8_t)ptr[1] << 16 | (uint32_t)(uint8_t)ptr[2] << 8  | (uint32_t)(uint8_t)ptr[3]); }
CLOD_INLINE static inline uint64_t beu40_dec(const char ptr[5]) { return (uint64_t)((uint64_t)(uint8_t)ptr[0] << 32 | (uint64_t)(uint8_t)ptr[1] << 24 | (uint64_t)(uint8_t)ptr[2] << 16 | (uint64_t)(uint8_t)ptr[3] << 8  | (uint64_t)(uint8_t)ptr[4]); }
CLOD_INLINE static inline uint64_t beu48_dec(const char ptr[6]) { return (uint64_t)((uint64_t)(uint8_t)ptr[0] << 40 | (uint64_t)(uint8_t)ptr[1] << 32 | (uint64_t)(uint8_t)ptr[2] << 24 | (uint64_t)(uint8_t)ptr[3] << 16 | (uint64_t)(uint8_t)ptr[4] << 8  | (uint64_t)(uint8_t)ptr[5]); }
CLOD_INLINE static inline uint64_t beu56_dec(const char ptr[7]) { return (uint64_t)((uint64_t)(uint8_t)ptr[0] << 48 | (uint64_t)(uint8_t)ptr[1] << 40 | (uint64_t)(uint8_t)ptr[2] << 32 | (uint64_t)(uint8_t)ptr[3] << 24 | (uint64_t)(uint8_t)ptr[4] << 16 | (uint64_t)(uint8_t)ptr[5] << 8  | (uint64_t)(uint8_t)ptr[6]); }
CLOD_INLINE static inline uint64_t beu64_dec(const char ptr[8]) { return (uint64_t)((uint64_t)(uint8_t)ptr[0] << 56 | (uint64_t)(uint8_t)ptr[1] << 48 | (uint64_t)(uint8_t)ptr[2] << 40 | (uint64_t)(uint8_t)ptr[3] << 32 | (uint64_t)(uint8_t)ptr[4] << 24 | (uint64_t)(uint8_t)ptr[5] << 16 | (uint64_t)(uint8_t)ptr[6] << 8  | (uint64_t)(uint8_t)ptr[7]); }

#define BEI8_MIN   INT8_C(-0x80)
#define BEI16_MIN INT16_C(-0x8000)
#define BEI24_MIN INT32_C(-0x800000)
#define BEI32_MIN INT32_C(-0x80000000)
#define BEI40_MIN INT64_C(-0x8000000000)
#define BEI48_MIN INT64_C(-0x800000000000)
#define BEI56_MIN INT64_C(-0x80000000000000)
#define BEI64_MIN INT64_C(-0x8000000000000000)

#define BEI8_MAX   INT8_C(0x7F)
#define BEI16_MAX INT16_C(0x7FFF)
#define BEI24_MAX INT32_C(0x7FFFFF)
#define BEI32_MAX INT32_C(0x7FFFFFFF)
#define BEI40_MAX INT64_C(0x7FFFFFFFFF)
#define BEI48_MAX INT64_C(0x7FFFFFFFFFFF)
#define BEI56_MAX INT64_C(0x7FFFFFFFFFFFFF)
#define BEI64_MAX INT64_C(0x7FFFFFFFFFFFFFFF)

CLOD_INLINE static inline void bei8_enc (char ptr[1], const int8_t  val) { beu8_enc (ptr, (uint8_t )(val)); }
CLOD_INLINE static inline void bei16_enc(char ptr[2], const int16_t val) { beu16_enc(ptr, (uint16_t)(val)); }
CLOD_INLINE static inline void bei24_enc(char ptr[3], const int32_t val) { beu24_enc(ptr, (uint32_t)(val > BEI24_MAX ? BEI24_MAX : val < BEI24_MIN ? BEI24_MIN : val)); }
CLOD_INLINE static inline void bei32_enc(char ptr[4], const int32_t val) { beu32_enc(ptr, (uint32_t)(val)); }
CLOD_INLINE static inline void bei40_enc(char ptr[5], const int64_t val) { beu40_enc(ptr, (uint64_t)(val > BEI40_MAX ? BEI40_MAX : val < BEI40_MIN ? BEI40_MIN : val)); }
CLOD_INLINE static inline void bei48_enc(char ptr[6], const int64_t val) { beu48_enc(ptr, (uint64_t)(val > BEI48_MAX ? BEI48_MAX : val < BEI48_MIN ? BEI48_MIN : val)); }
CLOD_INLINE static inline void bei56_enc(char ptr[7], const int64_t val) { beu56_enc(ptr, (uint64_t)(val > BEI56_MAX ? BEI56_MAX : val < BEI56_MIN ? BEI56_MIN : val)); }
CLOD_INLINE static inline void bei64_enc(char ptr[8], const int64_t val) { beu64_enc(ptr, (uint64_t)(val)); }

CLOD_INLINE static inline int8_t  bei8_dec (const char ptr[1]) { return (int8_t )(beu8_dec (ptr)); }
CLOD_INLINE static inline int16_t bei16_dec(const char ptr[2]) { return (int16_t)(beu16_dec(ptr)); }
CLOD_INLINE static inline int32_t bei24_dec(const char ptr[3]) { return (int32_t)(beu24_dec(ptr) << 8) >> 8; }
CLOD_INLINE static inline int32_t bei32_dec(const char ptr[4]) { return (int32_t)(beu32_dec(ptr)); }
CLOD_INLINE static inline int64_t bei40_dec(const char ptr[5]) { return (int64_t)(beu40_dec(ptr) << 24) >> 24; }
CLOD_INLINE static inline int64_t bei48_dec(const char ptr[6]) { return (int64_t)(beu48_dec(ptr) << 16) >> 16; }
CLOD_INLINE static inline int64_t bei56_dec(const char ptr[7]) { return (int64_t)(beu56_dec(ptr) << 8) >> 8; }
CLOD_INLINE static inline int64_t bei64_dec(const char ptr[8]) { return (int64_t)(beu64_dec(ptr)); }

CLOD_INLINE static inline void bef32_enc(char ptr[4], const float  f) { const union { float  f; uint32_t i; } u = { f }; beu32_enc(ptr, u.i); }
CLOD_INLINE static inline void bef64_enc(char ptr[8], const double f) { const union { double f; uint64_t i; } u = { f }; beu64_enc(ptr, u.i); }

CLOD_INLINE static inline float  bef32_dec(const char ptr[4]) { const union { float  f; uint32_t i; } u = { .i = beu32_dec(ptr) }; return u.f; }
CLOD_INLINE static inline double bef64_dec(const char ptr[8]) { const union { double f; uint64_t i; } u = { .i = beu64_dec(ptr) }; return u.f; }

#endif
