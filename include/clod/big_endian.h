/**
 * @file big_endian.h
 * @breif A silly header-only library for interacting with unaligned big-endian integers.
 *
 * @code
 *  beu24 three_byte_integer;
 *  three_byte_integer = beu24(5);
 *  uint32_t v = be(three_byte_integer);
 *  assert(v == 5);
 * @endcode
 */

#ifndef CLOD_BIG_ENDIAN_H
#define CLOD_BIG_ENDIAN_H

#include <clod/lib.h>
#include <limits.h>
#include <stdint.h>

static_assert(CHAR_BIT == 8);
static_assert(__STDC_VERSION__ >= 202311L, "signed integer shifting requires C23");
static_assert(__STDC_IEC_60559_BFP__, "bit representation of floats must be ICE 559");
static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);

typedef struct { uint8_t _b[1]; } bei8 ; static_assert(sizeof(bei8 ) * CHAR_BIT == 8  && alignof(bei8 ) == 1);
typedef struct { uint8_t _b[2]; } bei16; static_assert(sizeof(bei16) * CHAR_BIT == 16 && alignof(bei16) == 1);
typedef struct { uint8_t _b[3]; } bei24; static_assert(sizeof(bei24) * CHAR_BIT == 24 && alignof(bei24) == 1);
typedef struct { uint8_t _b[4]; } bei32; static_assert(sizeof(bei32) * CHAR_BIT == 32 && alignof(bei32) == 1);
typedef struct { uint8_t _b[5]; } bei40; static_assert(sizeof(bei40) * CHAR_BIT == 40 && alignof(bei40) == 1);
typedef struct { uint8_t _b[6]; } bei48; static_assert(sizeof(bei48) * CHAR_BIT == 48 && alignof(bei48) == 1);
typedef struct { uint8_t _b[7]; } bei56; static_assert(sizeof(bei56) * CHAR_BIT == 56 && alignof(bei56) == 1);
typedef struct { uint8_t _b[8]; } bei64; static_assert(sizeof(bei64) * CHAR_BIT == 64 && alignof(bei64) == 1);

#define bei8(val)  bei8_enc_ (val)
#define bei16(val) bei16_enc_(val)
#define bei24(val) bei24_enc_(val)
#define bei32(val) bei32_enc_(val)
#define bei40(val) bei40_enc_(val)
#define bei48(val) bei48_enc_(val)
#define bei56(val) bei56_enc_(val)
#define bei64(val) bei64_enc_(val)

CLOD_INLINE static inline bei8  bei8_enc_ (const int8_t  val) { bei8  n; n._b[0] = (uint8_t)(val); return n; }
CLOD_INLINE static inline bei16 bei16_enc_(const int16_t val) { bei16 n; n._b[0] = (uint8_t)(val >> 8 ); n._b[1] = (uint8_t)(val); return n; }
CLOD_INLINE static inline bei24 bei24_enc_(const int32_t val) { bei24 n; n._b[0] = (uint8_t)(val >> 16); n._b[1] = (uint8_t)(val >> 8 ); n._b[2] = (uint8_t)(val); return n; }
CLOD_INLINE static inline bei32 bei32_enc_(const int32_t val) { bei32 n; n._b[0] = (uint8_t)(val >> 24); n._b[1] = (uint8_t)(val >> 16); n._b[2] = (uint8_t)(val >> 8 ); n._b[3] = (uint8_t)(val); return n; }
CLOD_INLINE static inline bei40 bei40_enc_(const int64_t val) { bei40 n; n._b[0] = (uint8_t)(val >> 32); n._b[1] = (uint8_t)(val >> 24); n._b[2] = (uint8_t)(val >> 16); n._b[3] = (uint8_t)(val >> 8 ); n._b[4] = (uint8_t)(val); return n; }
CLOD_INLINE static inline bei48 bei48_enc_(const int64_t val) { bei48 n; n._b[0] = (uint8_t)(val >> 40); n._b[1] = (uint8_t)(val >> 32); n._b[2] = (uint8_t)(val >> 24); n._b[3] = (uint8_t)(val >> 16); n._b[4] = (uint8_t)(val >> 8 ); n._b[5] = (uint8_t)(val); return n; }
CLOD_INLINE static inline bei56 bei56_enc_(const int64_t val) { bei56 n; n._b[0] = (uint8_t)(val >> 48); n._b[1] = (uint8_t)(val >> 40); n._b[2] = (uint8_t)(val >> 32); n._b[3] = (uint8_t)(val >> 24); n._b[4] = (uint8_t)(val >> 16); n._b[5] = (uint8_t)(val >> 8 ); n._b[6] = (uint8_t)(val); return n; }
CLOD_INLINE static inline bei64 bei64_enc_(const int64_t val) { bei64 n; n._b[0] = (uint8_t)(val >> 56); n._b[1] = (uint8_t)(val >> 48); n._b[2] = (uint8_t)(val >> 40); n._b[3] = (uint8_t)(val >> 32); n._b[4] = (uint8_t)(val >> 24); n._b[5] = (uint8_t)(val >> 16); n._b[6] = (uint8_t)(val >> 8 ); n._b[7] = (uint8_t)(val); return n; }

CLOD_INLINE static inline int8_t  bei8_dec_ (const bei8  n) { return (int8_t)  n._b[0]; }
CLOD_INLINE static inline int16_t bei16_dec_(const bei16 n) { return (int16_t)((int16_t)(int8_t)n._b[0] << 8  | (int16_t)n._b[1]); }
CLOD_INLINE static inline int32_t bei24_dec_(const bei24 n) { return (int32_t)((int32_t)(int8_t)n._b[0] << 16 | (int32_t)n._b[1] << 8  | (int32_t)n._b[2]); }
CLOD_INLINE static inline int32_t bei32_dec_(const bei32 n) { return (int32_t)((int32_t)(int8_t)n._b[0] << 24 | (int32_t)n._b[1] << 16 | (int32_t)n._b[2] << 8  | (int32_t)n._b[3]); }
CLOD_INLINE static inline int64_t bei40_dec_(const bei40 n) { return (int64_t)((int64_t)(int8_t)n._b[0] << 32 | (int64_t)n._b[1] << 24 | (int64_t)n._b[2] << 16 | (int64_t)n._b[3] << 8  | (int64_t)n._b[4]); }
CLOD_INLINE static inline int64_t bei48_dec_(const bei48 n) { return (int64_t)((int64_t)(int8_t)n._b[0] << 40 | (int64_t)n._b[1] << 32 | (int64_t)n._b[2] << 24 | (int64_t)n._b[3] << 16 | (int64_t)n._b[4] << 8  | (int64_t)n._b[5]); }
CLOD_INLINE static inline int64_t bei56_dec_(const bei56 n) { return (int64_t)((int64_t)(int8_t)n._b[0] << 48 | (int64_t)n._b[1] << 40 | (int64_t)n._b[2] << 32 | (int64_t)n._b[3] << 24 | (int64_t)n._b[4] << 16 | (int64_t)n._b[5] << 8  | (int64_t)n._b[6]); }
CLOD_INLINE static inline int64_t bei64_dec_(const bei64 n) { return (int64_t)((int64_t)(int8_t)n._b[0] << 56 | (int64_t)n._b[1] << 48 | (int64_t)n._b[2] << 40 | (int64_t)n._b[3] << 32 | (int64_t)n._b[4] << 24 | (int64_t)n._b[5] << 16 | (int64_t)n._b[6] << 8  | (int64_t)n._b[7]); }

typedef struct { uint8_t _b[1]; } beu8 ; static_assert(sizeof(beu8 ) * CHAR_BIT == 8  && alignof(beu8 ) == 1);
typedef struct { uint8_t _b[2]; } beu16; static_assert(sizeof(beu16) * CHAR_BIT == 16 && alignof(beu16) == 1);
typedef struct { uint8_t _b[3]; } beu24; static_assert(sizeof(beu24) * CHAR_BIT == 24 && alignof(beu24) == 1);
typedef struct { uint8_t _b[4]; } beu32; static_assert(sizeof(beu32) * CHAR_BIT == 32 && alignof(beu32) == 1);
typedef struct { uint8_t _b[5]; } beu40; static_assert(sizeof(beu40) * CHAR_BIT == 40 && alignof(beu40) == 1);
typedef struct { uint8_t _b[6]; } beu48; static_assert(sizeof(beu48) * CHAR_BIT == 48 && alignof(beu48) == 1);
typedef struct { uint8_t _b[7]; } beu56; static_assert(sizeof(beu56) * CHAR_BIT == 56 && alignof(beu56) == 1);
typedef struct { uint8_t _b[8]; } beu64; static_assert(sizeof(beu64) * CHAR_BIT == 64 && alignof(beu64) == 1);

#define beu8(val)  beu8_enc_ (val)
#define beu16(val) beu16_enc_(val)
#define beu24(val) beu24_enc_(val)
#define beu32(val) beu32_enc_(val)
#define beu40(val) beu40_enc_(val)
#define beu48(val) beu48_enc_(val)
#define beu56(val) beu56_enc_(val)
#define beu64(val) beu64_enc_(val)

CLOD_INLINE static inline beu8  beu8_enc_ (const uint8_t  val) { beu8  n; n._b[0] = (uint8_t)(val); return n; }
CLOD_INLINE static inline beu16 beu16_enc_(const uint16_t val) { beu16 n; n._b[0] = (uint8_t)(val >> 8) ; n._b[1] = (uint8_t)(val); return n; }
CLOD_INLINE static inline beu24 beu24_enc_(const uint32_t val) { beu24 n; n._b[0] = (uint8_t)(val >> 16); n._b[1] = (uint8_t)(val >> 8 ); n._b[2] = (uint8_t)(val); return n; }
CLOD_INLINE static inline beu32 beu32_enc_(const uint32_t val) { beu32 n; n._b[0] = (uint8_t)(val >> 24); n._b[1] = (uint8_t)(val >> 16); n._b[2] = (uint8_t)(val >> 8 ); n._b[3] = (uint8_t)(val); return n; }
CLOD_INLINE static inline beu40 beu40_enc_(const uint64_t val) { beu40 n; n._b[0] = (uint8_t)(val >> 32); n._b[1] = (uint8_t)(val >> 24); n._b[2] = (uint8_t)(val >> 16); n._b[3] = (uint8_t)(val >> 8 ); n._b[4] = (uint8_t)(val); return n; }
CLOD_INLINE static inline beu48 beu48_enc_(const uint64_t val) { beu48 n; n._b[0] = (uint8_t)(val >> 40); n._b[1] = (uint8_t)(val >> 32); n._b[2] = (uint8_t)(val >> 24); n._b[3] = (uint8_t)(val >> 16); n._b[4] = (uint8_t)(val >> 8 ); n._b[5] = (uint8_t)(val); return n; }
CLOD_INLINE static inline beu56 beu56_enc_(const uint64_t val) { beu56 n; n._b[0] = (uint8_t)(val >> 48); n._b[1] = (uint8_t)(val >> 40); n._b[2] = (uint8_t)(val >> 32); n._b[3] = (uint8_t)(val >> 24); n._b[4] = (uint8_t)(val >> 16); n._b[5] = (uint8_t)(val >> 8 ); n._b[6] = (uint8_t)(val); return n; }
CLOD_INLINE static inline beu64 beu64_enc_(const uint64_t val) { beu64 n; n._b[0] = (uint8_t)(val >> 56); n._b[1] = (uint8_t)(val >> 48); n._b[2] = (uint8_t)(val >> 40); n._b[3] = (uint8_t)(val >> 32); n._b[4] = (uint8_t)(val >> 24); n._b[5] = (uint8_t)(val >> 16); n._b[6] = (uint8_t)(val >> 8 ); n._b[7] = (uint8_t)(val); return n; }

CLOD_INLINE static inline uint8_t  beu8_dec_ (const beu8  n) { return (uint8_t)n._b[0]; }
CLOD_INLINE static inline uint16_t beu16_dec_(const beu16 n) { return (uint16_t)((uint16_t)n._b[0] << 8  | (uint16_t)n._b[1]); }
CLOD_INLINE static inline uint32_t beu24_dec_(const beu24 n) { return (uint32_t)((uint32_t)n._b[0] << 16 | (uint32_t)n._b[1] << 8  | (uint32_t)n._b[2]); }
CLOD_INLINE static inline uint32_t beu32_dec_(const beu32 n) { return (uint32_t)((uint32_t)n._b[0] << 24 | (uint32_t)n._b[1] << 16 | (uint32_t)n._b[2] << 8  | (uint32_t)n._b[3]); }
CLOD_INLINE static inline uint64_t beu40_dec_(const beu40 n) { return (uint64_t)((uint64_t)n._b[0] << 32 | (uint64_t)n._b[1] << 24 | (uint64_t)n._b[2] << 16 | (uint64_t)n._b[3] << 8  | (uint64_t)n._b[4]); }
CLOD_INLINE static inline uint64_t beu48_dec_(const beu48 n) { return (uint64_t)((uint64_t)n._b[0] << 40 | (uint64_t)n._b[1] << 32 | (uint64_t)n._b[2] << 24 | (uint64_t)n._b[3] << 16 | (uint64_t)n._b[4] << 8  | (uint64_t)n._b[5]); }
CLOD_INLINE static inline uint64_t beu56_dec_(const beu56 n) { return (uint64_t)((uint64_t)n._b[0] << 48 | (uint64_t)n._b[1] << 40 | (uint64_t)n._b[2] << 32 | (uint64_t)n._b[3] << 24 | (uint64_t)n._b[4] << 16 | (uint64_t)n._b[5] << 8  | (uint64_t)n._b[6]); }
CLOD_INLINE static inline uint64_t beu64_dec_(const beu64 n) { return (uint64_t)((uint64_t)n._b[0] << 56 | (uint64_t)n._b[1] << 48 | (uint64_t)n._b[2] << 40 | (uint64_t)n._b[3] << 32 | (uint64_t)n._b[4] << 24 | (uint64_t)n._b[5] << 16 | (uint64_t)n._b[6] << 8  | (uint64_t)n._b[7]); }

typedef struct { uint8_t _b[4]; } bef32; static_assert(sizeof(bef32) * CHAR_BIT == 32 && alignof(bef32) == 1);
typedef struct { uint8_t _b[8]; } bef64; static_assert(sizeof(bef64) * CHAR_BIT == 64 && alignof(bef64) == 1);

#define bef32(val) bef32_enc_(val)
#define bef64(val) bef64_enc_(val)

CLOD_INLINE static inline bef32 bef32_enc_(const float  f) { const union { float  f; uint32_t i; } u = { f }; bef32 n; n._b[0] = (uint8_t)(u.i >> 24); n._b[1] = (uint8_t)(u.i >> 16); n._b[2] = (uint8_t)(u.i >> 8) ; n._b[3] = (uint8_t)(u.i); return n; }
CLOD_INLINE static inline bef64 bef64_enc_(const double f) { const union { double f; uint64_t i; } u = { f }; bef64 n; n._b[0] = (uint8_t)(u.i >> 56); n._b[1] = (uint8_t)(u.i >> 48); n._b[2] = (uint8_t)(u.i >> 40); n._b[3] = (uint8_t)(u.i >> 32); n._b[4] = (uint8_t)(u.i >> 24); n._b[5] = (uint8_t)(u.i >> 16); n._b[6] = (uint8_t)(u.i >> 8); n._b[7] = (uint8_t)(u.i); return n; }

CLOD_INLINE static inline float  bef32_dec_(const bef32 n) { const union { float  f; uint32_t i; } u = { .i = (uint32_t)n._b[0] << 24 | (uint32_t)n._b[1] << 16 | (uint32_t)n._b[2] << 8  | (uint32_t)n._b[3] }; return u.f; }
CLOD_INLINE static inline double bef64_dec_(const bef64 n) { const union { double f; uint64_t i; } u = { .i = (uint64_t)n._b[0] << 56 | (uint64_t)n._b[1] << 48 | (uint64_t)n._b[2] << 40 | (uint64_t)n._b[3] << 32 | (uint64_t)n._b[4] << 24 | (uint64_t)n._b[5] << 16 | (uint64_t)n._b[6] << 8 | (uint64_t)n._b[7] }; return u.f; }

#define be(be) _Generic((be),\
	bei8:  bei8_dec_ ,\
	bei16: bei16_dec_,\
	bei24: bei24_dec_,\
	bei32: bei32_dec_,\
	bei40: bei40_dec_,\
	bei48: bei48_dec_,\
	bei56: bei56_dec_,\
	bei64: bei64_dec_,\
\
	beu8:  beu8_dec_ ,\
	beu16: beu16_dec_,\
	beu24: beu24_dec_,\
	beu32: beu32_dec_,\
	beu40: beu40_dec_,\
	beu48: beu48_dec_,\
	beu56: beu56_dec_,\
	beu64: beu64_dec_,\
\
	bef32: bef32_dec_,\
	bef64: bef64_dec_ \
)(be)

#endif
