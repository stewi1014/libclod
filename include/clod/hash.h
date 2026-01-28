/**
 * @file clod/hash.h
 * @defgroup hash Hash methods
 *
 * The general idea is each hash method provides an init/add/finalise method.
 * Initialisation creates a hash state which is initialised to some constant optionally with a seed,
 * adding is the meat of the implementation and updates the state with new data, and
 * finalising parses the state to produce a final output.
 * In the case of CRC, the state is the CRC itself less some simple xor finalisation for some variants,
 * but this is not always the case (i.e. sip64).
 *
 * The CRC methods use lookup tables. See libclod/src/hash/crc_tables_generate.c for how they are generated,
 * and libclod/src/hash/crc.c for how they are used.
 *
 * @code
 * state = clod_<alg>_init([seed]);
 * state = clod_<alg>_add(state, data, size);
 * state = clod_<alg>_add(state, data2, size2);
 * state = clod_<alg>_add(state, data3, size3);
 * hash = clod_<alg>_finalise(state);
 * @endcode
 *
 * Often, using streaming methods is an annoying complexity,
 * so a method for computing the checksum of a single blob of data is provided.
 *
 * @code
 * hash = clod_<alg>([seed], data, size)
 * @endcode
 *
 * @{
 */
#ifndef CLOD_HASH_H
#define CLOD_HASH_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint64_t _v0;
	uint64_t _v1;
	uint64_t _v2;
	uint64_t _v3;
	uint8_t _size;
	unsigned char _buf[7];
} clod_sip64_state;

/**
 * Initialise a sip64 hash state.
 * @param[in] seed Seed value for the hash.
 */
#define clod_sip64_init(seed) ((clod_sip64_state){\
	._v0 = UINT64_C(0x736f6d6570736575) ^ seed,\
	._v1 = UINT64_C(0x646f72616e646f6d) ^ seed,\
	._v2 = UINT64_C(0x6c7967656e657261) ^ seed,\
	._v3 = UINT64_C(0x7465646279746573) ^ seed,\
	._size = 0\
})

/**
 * Add data to a sip64 hash.
 * SipHash is aimed at maximising speed and uniformity of entropy across the 64-bit range.
 *
 * @param[in] state Hash state.
 * @param[in] data Value to be hashed.
 * @param[in] size Size of data.
 * @return Updated state.
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE CLOD_NONNULL(2)
clod_sip64_state clod_sip64_add(clod_sip64_state state, const void *data, size_t size);

/**
 * Finalise a sip64 hash.
 * @param[in] state Hash state.
 * @return 64-bit hash value.
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE
uint64_t clod_sip64_finalise(clod_sip64_state state);

#define clod_sip64(seed, data, size) clod_sip64_finalise(clod_sip64_add(clod_sip64_init(seed), data, size))


/**
 * Polynomial: 0x42F0E1EBA9EA3693
 * Reflected: false
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE CLOD_NONNULL(2)
uint64_t
clod_crc64_add(uint64_t crc, const void *data, size_t data_len);
#define clod_crc64_init() UINT64_C(0x0)
#define clod_crc64_finalise(crc) ((uint64_t)((uint64_t)crc ^ UINT64_C(0x0)))
#define clod_crc64(data, size) clod_crc64_finalise(clod_crc64_add(clod_crc64_init(), data, size))

/**
 * Polynomial: 0x4C11DB7
 * Reflected: true
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE CLOD_NONNULL(2)
uint32_t clod_crc32_add(uint32_t crc, const void *data, size_t data_len);
#define clod_crc32_init() UINT32_C(0xFFFFFFFF)
#define clod_crc32_finalise(crc) ((uint32_t)((uint32_t)crc ^ UINT32_C(0xFFFFFFFF)))
#define clod_crc32(data, size) clod_crc32_finalise(clod_crc32_add(clod_crc32_init(), data, size))

/**
 * Polynomial: 0x864CFB
 * Reflected: false
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE CLOD_NONNULL(2)
uint32_t clod_crc24_add(uint32_t crc, const void *data, size_t data_len);
#define clod_crc24_init() UINT32_C(0xB704CE)
#define clod_crc24_finalise(crc) ((uint32_t)((uint32_t)crc ^ UINT32_C(0x0)))
#define clod_crc24(data, size) clod_crc24_finalise(clod_crc24_add(clod_crc24_init(), data, size))

/**
 * Polynomial: 0x1021
 * Reflected: true
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE CLOD_NONNULL(2)
uint16_t clod_crc16_add(uint16_t crc, const void *data, size_t data_len);
#define clod_crc16_init() UINT16_C(0x0)
#define clod_crc16_finalise(crc) ((uint16_t)((uint16_t)crc ^ UINT16_C(0x0)))
#define clod_crc16(data, size) clod_crc16_finalise(clod_crc16_add(clod_crc16_init(), data, size))

/**
 * Polynomial: 0x7
 * Reflected: false
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE CLOD_NONNULL(2)
uint8_t clod_crc8_add(uint8_t crc, const void *data, size_t data_len);
#define clod_crc8_init() UINT8_C(0x0)
#define clod_crc8_finalise(crc) ((uint8_t)((uint8_t)crc ^ UINT8_C(0x0)))
#define clod_crc8(data, size) clod_crc8_finalise(clod_crc8_add(clod_crc8_init(), data, size))

/** @} */
#endif
