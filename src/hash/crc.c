#include "config.h"
#include <clod/hash.h>
#include <stddef.h>
#include <stdint.h>

#include "crc_tables.h"

#if CLOD_HAVE_X86_64
#include <immintrin.h>
#endif

#ifdef __GNUC__
#define HAVE_CRC32_INTRINSIC __builtin_cpu_supports("sse4.2")
#else
#define HAVE_CRC32_INTRINSIC 0
#endif

CLOD_CONST
static uint64_t gf2_mul_mod(uint64_t a, const uint64_t b, const uint64_t polynomial, const uint8_t bits) {
	uint64_t res = 0;
	for (uint8_t i = 0; i < bits; i++) {
		if (b & UINT64_C(1) << i) {
			res ^= a;
		}

		if (a & UINT64_C(1) << (bits - 1)) {
			a = a << 1 ^ polynomial;
		} else {
			a = a << 1;
		}
	}
	return res & (bits == 64 ? UINT64_C(-1) : (UINT64_C(1) << bits) - 1);
}

CLOD_CONST
static uint64_t gf2_mul_mod_reflected(uint64_t a, const uint64_t b, const uint64_t polynomial, const uint8_t bits) {
	uint64_t res = 0;
	for (uint8_t i = 0; i < bits; i++) {
		if (b & UINT64_C(1) << i) {
			res ^= a;
		}

		if (a & 1) {
			a = a >> 1 ^ polynomial;
		} else {
			a >>= 1;
		}
	}
	return res;
}

uint64_t clod_crc64_add(uint64_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *restrict bytes = data;
	if (bytes) {
		for (size_t i = 0; i < data_len; i++) {
			crc = crc64_table[(crc >> 56) ^ bytes[i]] ^ crc << 8;
		}
	} else {
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1) {
			if (data_len & 1) {
				crc = gf2_mul_mod(crc, crc64_power_table[b], CRC64_NORMALISED_POLYNOMIAL, 64);
			}
		}
	}
	return crc;
}

uint32_t clod_crc32_add(uint32_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *restrict bytes = data;
	if (bytes) {
		#if CLOD_HAVE_X86_64
		if (HAVE_CRC32_INTRINSIC) {
			size_t i = 0;
			while (((uintptr_t)data + i ) % 8 != 0 && i < data_len) {
				crc = _mm_crc32_u8(crc, bytes[i++]);
			}

			uint64_t crc64 = crc;
			for (; i + 8 <= data_len; i += 8) {
				crc64 = _mm_crc32_u64(crc64, *(uint64_t*)((char*)data + i));
			}

			crc = (uint32_t)crc64;
			while (i < data_len) {
				crc = _mm_crc32_u8(crc, bytes[i++]);
			}

			return crc;
		}
		#endif
		for (size_t i = 0; i < data_len; i++) {
			crc = crc32_table[(crc & 0xff) ^ bytes[i]] ^ crc >> 8;
		}
	} else {
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1) {
			if (data_len & 1) {
				crc = (uint32_t)gf2_mul_mod_reflected(crc, crc32_power_table[b], CRC32_NORMALISED_POLYNOMIAL, 32);
			}
		}
	}
	return crc;
}

uint32_t clod_crc24_add(uint32_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *restrict bytes = data;
	crc = crc & 0x00FFFFFF;
	if (bytes) {
		for (size_t i = 0; i < data_len; i++) {
			crc = (crc24_table[(crc >> 16) ^ bytes[i]] ^ crc << 8) & 0x00FFFFFF;
		}
	} else {
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1) {
			if (data_len & 1) {
				crc = (uint32_t)gf2_mul_mod(crc, crc24_power_table[b], CRC24_NORMALISED_POLYNOMIAL, 24);
			}
		}
	}
	return crc;
}

uint16_t clod_crc16_add(uint16_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *restrict bytes = data;
	if (bytes) {
		for (size_t i = 0; i < data_len; i++) {
			crc = crc16_table[(crc & 0xff) ^ bytes[i]] ^ (uint16_t)(crc >> 8);
		}
	} else {
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1) {
			if (data_len & 1) {
				crc = (uint16_t)gf2_mul_mod_reflected(crc, crc16_power_table[b], CRC16_NORMALISED_POLYNOMIAL, 16);
			}
		}
	}
	return crc;
}

uint8_t clod_crc8_add(uint8_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *restrict bytes = data;
	if (bytes) {
		for (size_t i = 0; i < data_len; i++) {
			crc = crc8_table[(crc & 0xff) ^ bytes[i]] ^ (uint8_t)(crc >> 8);
		}
	} else {
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1) {
			if (data_len & 1) {
				crc = (uint8_t)gf2_mul_mod(crc, crc8_power_table[b], CRC8_NORMALISED_POLYNOMIAL, 8);
			}
		}
	}
	return crc;
}
