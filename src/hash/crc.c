#include <clod/hash.h>
#include <stddef.h>
#include <stdint.h>

#include "crc_tables.h"

CLOD_CONST
static uint64_t gf2_mul_mod(uint64_t a, uint64_t b, uint64_t polynomial, uint8_t bits) {
	uint64_t res = 0;
	for (uint8_t i = 0; i < bits; i++) {
		if (b & UINT64_C(1) << i)
			res ^= a;
		if (a & UINT64_C(1) << (bits - 1))
			a = a << 1 ^ polynomial;
		else
			a = a << 1;
	}
	return res & (bits == 64 ? UINT64_C(-1) : (UINT64_C(1) << bits) - 1);
}

CLOD_CONST
static uint64_t gf2_mul_mod_reflected(uint64_t a, uint64_t b, uint64_t polynomial, uint8_t bits) {
	uint64_t res = 0;
	for (uint8_t i = 0; i < bits; i++) {
		if (b & UINT64_C(1) << i)
			res ^= a;
		if (a & 1)
			a = a >> 1 ^ polynomial;
		else
			a >>= 1;
	}
	return res;
}

uint64_t clod_crc64_add(uint64_t crc, const uint8_t *restrict data, size_t data_len) {
	if (data)
		for (size_t i = 0; i < data_len; i++)
			crc = crc64_table[(crc >> 56) ^ data[i]] ^ crc << 8;
	else
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1)
			if (data_len & 1)
				crc = gf2_mul_mod(crc, crc64_power_table[b], CRC64_NORMALISED_POLYNOMIAL, 64);
	return crc;
}

uint32_t clod_crc32_add(uint32_t crc, const uint8_t *restrict data, size_t data_len) {
	if (data)
		for (size_t i = 0; i < data_len; i++)
			crc = crc32_table[(crc & 0xff) ^ data[i]] ^ crc >> 8;
	else
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1)
			if (data_len & 1)
				crc = (uint32_t)gf2_mul_mod_reflected(crc, crc32_power_table[b], CRC32_NORMALISED_POLYNOMIAL, 32);
	return crc;
}

uint32_t clod_crc24_add(uint32_t crc, const uint8_t *restrict data, size_t data_len) {
	crc = crc & 0x00FFFFFF;
	if (data)
		for (size_t i = 0; i < data_len; i++)
			crc = (crc24_table[(crc >> 16) ^ data[i]] ^ crc << 8) & 0x00FFFFFF;
	else
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1)
			if (data_len & 1)
				crc = (uint32_t)gf2_mul_mod(crc, crc24_power_table[b], CRC24_NORMALISED_POLYNOMIAL, 24);
	return crc;
}

uint16_t clod_crc16_add(uint16_t crc, const uint8_t *restrict data, size_t data_len) {
	if (data)
		for (size_t i = 0; i < data_len; i++)
			crc = crc16_table[(crc & 0xff) ^ data[i]] ^ (uint16_t)(crc >> 8);
	else
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1)
			if (data_len & 1)
				crc = (uint16_t)gf2_mul_mod_reflected(crc, crc16_power_table[b], CRC16_NORMALISED_POLYNOMIAL, 16);
	return crc;
}

uint8_t clod_crc8_add(uint8_t crc, const uint8_t *restrict data, size_t data_len) {
	if (data)
		for (size_t i = 0; i < data_len; i++)
			crc = crc8_table[(crc & 0xff) ^ data[i]] ^ (uint8_t)(crc >> 8);
	else
		for (uint8_t b = 0; data_len > 0; b++, data_len >>= 1)
			if (data_len & 1)
				crc = (uint8_t)gf2_mul_mod(crc, crc8_power_table[b], CRC8_NORMALISED_POLYNOMIAL, 8);
	return crc;
}
