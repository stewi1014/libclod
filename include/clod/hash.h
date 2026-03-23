/**
 * @file clod/hash.h
 * @defgroup hash Hash methods
 *
 * The general idea is each hash method provides an init/add/finalise method.
 * Initialisation creates a hash state which is initialised to some constant optionally including a seed value.
 * Adding is the meat of the implementation and updates the state with new data.
 * Finalisation parses the state to produce a final output.
 * In the case of CRC, finalisation is a single xor with a constant, which can be undone to produce
 * the hash state as it was before finalisation by simply performing finalisation a second time.
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
 * In addition, the crc add methods can be provided a null data argument to skip the crc ahead so many bytes.
 * This enables some fun gymnastics such as the offset add methods.
 * One can create a crc for some large data set without actually reading or hashing it,
 * and then add different parts of the data later - sparse data sets can be hashed efficiently.
 *
 * @code
 * state = clod_<alg>_init();
 * state = clod_<alg>_add(state, 10000); // Fast for any size
 * state = clod_<alg>_add_at(state, data, 1000, 9000);
 * state = clod_<alg>_add_at(state, data3, 2000, 0);
 * state = clod_<alg>_add_at(state, data2, 7000, 2000);
 * crc = clod_<alg>_finalise(state);
 * @endcode
 *
 * Or, an already computed crc can be updated when a small portion of the original data is modified
 * without needing to hash anything other than the modified section.
 *
 * @code
 * state = clod_<alg>_finalise(old_crc);
 * state = clod_<alg>_add_at(state, old_data, 1000, 9000); // Adding the old data again xors it to zero.
 * state = clod_<alg>_add_at(state, new_data, 1000, 9000);
 * new_crc = clod_<alg>_finalise(state);
 * @endcode
 *
 * @{
 */
#ifndef LIBCLOD_HASH_H
#define LIBCLOD_HASH_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

/**
 * SipHash state.
 * Used to hold buffers during streamed hashing.
 */
typedef struct {
	uint64_t _v0;
	uint64_t _v1;
	uint64_t _v2;
	uint64_t _v3;
	uint8_t _size;
	unsigned char _buf[7];
} clod_sip64_state;

/**
 * Initialise the hash state.
 *
 * @param[in] seed Seed value for the hash
 */
#define clod_sip64_init(seed) ((clod_sip64_state){\
	._v0 = UINT64_C(0x736f6d6570736575) ^ seed,\
	._v1 = UINT64_C(0x646f72616e646f6d) ^ seed,\
	._v2 = UINT64_C(0x6c7967656e657261) ^ seed,\
	._v3 = UINT64_C(0x7465646279746573) ^ seed,\
	._size = 0\
})

/**
 * Add data to a SipHash state.
 *
 * This is a modified variant of SipHash to support streaming,
 * so results will be incompatible with the reference implementation.
 *
 * @param[in] state Hash state
 * @param[in] data Value to be hashed
 * @param[in] size Size of data
 * @return Updated state
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE CLOD_NONNULL(2)
clod_sip64_state clod_sip64_add(clod_sip64_state state, const void *data, size_t size);

/**
 * Finalise a SipHash state.
 *
 * This is a modified variant of SipHash to support streaming,
 * so results will be incompatible with the reference implementation.
 *
 * @param[in] state Hash state
 * @return 64-bit hash value
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE
uint64_t clod_sip64_finalise(clod_sip64_state state);

/**
 * One-shot a SipHash result.
 *
 * This is a modified variant of SipHash to support streaming,
 * so results will be incompatible with the reference implementation.
 *
 * @param[in] seed Seed value for the hash
 * @param[in] data Value to be hashed
 * @param[in] size Size of \p data
 * @return 64-bit hash value
 */
#define clod_sip64(seed, data, size) clod_sip64_finalise(clod_sip64_add(clod_sip64_init(seed), data, size))

/**
 * Initialise a crc64 hash state.
 */
#define clod_crc64_init() UINT64_C(0xFFFFFFFFFFFFFFFF)

/**
 * Add data to the hash state.
 * If data is null it functions as if data was zeroed,
 * but can skip almost all the work.
 *
 * Polynomial: 0x42F0E1EBA9EA3693
 * Reflected: false
 *
 * @param[in] crc Hash state
 * @param[in] data (nullable) Data to add to hash state
 * @param[in] data_len Size of \p data
 * @return Updated hash state
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE
uint64_t clod_crc64_add(uint64_t crc, const void *data, size_t data_len);

/**
 * Add data to the hash state at an offset.
 * The added data is conceptually xored with any existing data.
 *
 * @param[in] crc Hash state
 * @param[in] data The data to add to the hash state
 * @param[in] data_size The size of \p data
 * @param[in] offset The offset of data from the end of the hashed data
 * @return The crc hash state with the given data added
 */
#define clod_crc64_add_at(crc, data, data_size, offset) \
	(crc ^ clod_crc64_add(clod_crc64_add(0, data, data_size), nullptr, offset))

/**
 * Finalise a crc64 hash.
 *
 * @param[in] crc Hash state
 * @return 64-bit hash value
 */
#define clod_crc64_finalise(crc) (crc ^ UINT64_C(0xFFFFFFFFFFFFFFFF))

/**
 * One-shot a crc64 hash.
 *
 * @param[in] data Data to hash
 * @param[in] size Size of \p data
 * @return 64-bit hash value
 */
#define clod_crc64(data, size) clod_crc64_finalise(clod_crc64_add(clod_crc64_init(), data, size))

/**
 * Initialise a crc32 hash state.
 */
#define clod_crc32_init() UINT32_C(0xFFFFFFFF)

/**
 * Add data to the hash state.
 * If data is null it functions as if data was zeroed,
 * but can skip almost all the work.
 *
 * This method uses CPU intrinsics for CRC generation when possible.
 *
 * Polynomial: 0x1EDC6F41
 * Reflected: true
 *
 * @param[in] crc Hash state
 * @param[in] data (nullable) Data to add to hash state
 * @param[in] data_len Size of \p data
 * @return Updated hash state
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE
uint32_t clod_crc32_add(uint32_t crc, const void *data, size_t data_len);

/**
 * Add data to the hash state at an offset.
 * The added data is conceptually xored with any existing data.
 *
 * @param[in] crc Hash state
 * @param[in] data The data to add to the hash state
 * @param[in] data_size The size of \p data
 * @param[in] offset The offset of data from the end of the hashed data
 * @return The crc hash state with the given data added
 */
#define clod_crc32_add_at(crc, data, data_size, offset) \
	(crc ^ clod_crc32_add(clod_crc32_add(0, data, data_size), nullptr, offset))

/**
 * Finalise a crc32 hash state.
 *
 * @param[in] crc Hash state
 * @return 32-bit hash value
 */
#define clod_crc32_finalise(crc) (crc ^ UINT32_C(0xFFFFFFFF))

/**
 * One-shot a crc32 hash.
 *
 * @param[in] data Data to hash
 * @param[in] size Size of \p data
 * @return 32-bit hash value
 */
#define clod_crc32(data, size) clod_crc32_finalise(clod_crc32_add(clod_crc32_init(), data, size))

/**
 * Initialise a crc24 hash state.
 */
#define clod_crc24_init() UINT32_C(0xFFFFFF)

/**
 * Add data to the hash state.
 * If data is null it functions as if data was zeroed,
 * but can skip almost all the work.
 *
 * Polynomial: 0x864CFB
 * Reflected: false
 *
 * @param[in] crc Hash state
 * @param[in] data (nullable) Data to add to hash state
 * @param[in] data_len Size of \p data
 * @return Updated hash state
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE
uint32_t clod_crc24_add(uint32_t crc, const void *data, size_t data_len);

/**
 * Add data to the hash state at an offset.
 * The added data is conceptually xored with any existing data.
 *
 * @param[in] crc Hash state
 * @param[in] data The data to add to the hash state
 * @param[in] data_size The size of \p data
 * @param[in] offset The offset of data from the end of the hashed data
 * @return The crc hash state with the given data added
 */
#define clod_crc24_add_at(crc, data, data_size, offset) \
	(crc ^ clod_crc24_add(clod_crc24_add(0, data, data_size), nullptr, offset))

/**
 * Finalise a crc24 hash state.
 *
 * @param[in] crc Hash state
 * @return 24-bit hash value
 */
#define clod_crc24_finalise(crc) (crc ^ UINT32_C(0xFFFFFF))

/**
 * One-shot a crc24 hash.
 *
 * @param[in] data Data to hash
 * @param[in] size Size of \p data
 * @return 24-bit hash value
 */
#define clod_crc24(data, size) clod_crc24_finalise(clod_crc24_add(clod_crc24_init(), data, size))

/**
 * Initialise a crc16 hash state.
 */
#define clod_crc16_init() UINT16_C(0xFFFF)

/**
 * Add data to the hash state.
 * If data is null it functions as if data was zeroed,
 * but can skip almost all the work.
 *
 * Polynomial: 0x1021
 * Reflected: true
 *
 * @param[in] crc Hash state
 * @param[in] data (nullable) Data to add to hash state
 * @param[in] data_len Size of \p data
 * @return Updated hash state
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE
uint16_t clod_crc16_add(uint16_t crc, const void *data, size_t data_len);

/**
 * Add data to the hash state at an offset.
 * The added data is conceptually xored with any existing data.
 *
 * @param[in] crc Hash state
 * @param[in] data The data to add to the hash state
 * @param[in] data_size The size of \p data
 * @param[in] offset The offset of data from the end of the hashed data
 * @return The crc hash state with the given data added
 */
#define clod_crc16_add_at(crc, data, data_size, offset) \
	(crc ^ clod_crc16_add(clod_crc16_add(0, data, data_size), nullptr, offset))

/**
 * Finalise a crc16 hash state.
 *
 * @param[in] crc Hash state
 * @return 16-bit hash value
 */
#define clod_crc16_finalise(crc) (crc ^ UINT16_C(0xFFFF))

/**
 * One-shot a crc16 hash.
 *
 * @param[in] data Data to hash
 * @param[in] size Size of \p data
 * @return 16-bit hash value
 */
#define clod_crc16(data, size) clod_crc16_finalise(clod_crc16_add(clod_crc16_init(), data, size))

/**
 * Initialise a crc8 hash state.
 */
#define clod_crc8_init() UINT8_C(0xFF)

/**
 * Add data to the hash state.
 * If data is null it functions as if data was zeroed,
 * but can skip almost all the work.
 *
 * Polynomial: 0x7
 * Reflected: false
 *
 * @param[in] crc Hash state
 * @param[in] data (nullable) Data to add to hash state
 * @param[in] data_len Size of \p data
 * @return Updated hash state
 */
CLOD_API CLOD_USE_RETURN CLOD_PURE
uint8_t clod_crc8_add(uint8_t crc, const void *data, size_t data_len);

/**
 * Add data to the hash state at an offset.
 * The added data is conceptually xored with any existing data.
 *
 * @param[in] crc Hash state
 * @param[in] data The data to add to the hash state
 * @param[in] data_size The size of \p data
 * @param[in] offset The offset of data from the end of the hashed data
 * @return The crc hash state with the given data added
 */
#define clod_crc8_add_at(crc, data, data_size, offset) \
	(crc ^ clod_crc8_add(clod_crc8_add(0, data, data_size), nullptr, offset))

/**
 * Finalise a crc8 hash state.
 *
 * @param[in] crc Hash state
 * @return 8-bit hash value
 */
#define clod_crc8_finalise(crc) (crc ^ UINT8_C(0xFF))

/**
 * One-shot a crc8 hash.
 *
 * @param[in] data Data to hash
 * @param[in] size Size of \p data
 * @return 8-bit hash value
 */
#define clod_crc8(data, size) clod_crc8_finalise(clod_crc8_add(clod_crc8_init(), data, size))

/** @} */
#endif
