#include "test.h"
#include <stdint.h>
#include <string.h>
#include <clod/endian_big.h>
#include <clod/endian_little.h>

const struct {
	uint64_t n;
	uint8_t data_size;
	uint8_t data[10];
} le_unsigned_tests[] = {
	{UINT64_MAX,        10,{-1, -1, -1, -1, -1, -1, -1, -1, -1, 1}},
	{UINT64_MAX - 1,    10,{-2, -1, -1, -1, -1, -1, -1, -1, -1, 1}},
	{UINT64_MAX >> 1,   9, {-1, -1, -1, -1, -1, -1, -1, -1, 127}},
	{UINT64_C(1) << 63, 10,{-128, -128, -128, -128, -128, -128, -128, -128, -128, 1}},
	{127, 1, {127}},
	{128, 2, {-128, 1}}
};

const struct {
	int64_t n;
	uint8_t data_size;
	uint8_t data[10];
} le_signed_tests[] = {
	{ INT64_MAX,        10,{-1, -1, -1, -1, -1, -1, -1, -1, -1, 0}},
	{ INT64_MIN,        10,{-128, -128, -128, -128, -128, -128, -128, -128, -128, 1}},
	{ INT64_MAX - 1,    10,{-2, -1, -1, -1, -1, -1, -1, -1, -1, 0}},
	{ INT64_MAX >> 1,   9, {-1, -1, -1, -1, -1, -1, -1, -1, 63}},
	{ INT64_MIN + 1,    10,{-127, -128, -128, -128, -128, -128, -128, -128, -128, 1}},
	{ INT64_C(1) << 62, 10,{-128, -128, -128, -128, -128, -128, -128, -128, -64, 0}}
};

const struct {
	uint64_t n;
	uint8_t data_size;
	uint8_t data[10];
} be_unsigned_tests[] = {
	{UINT64_MAX,        10,{-127, -1, -1, -1, -1, -1, -1, -1, -1, 127}},
	{UINT64_MAX - 1,    10,{-127, -1, -1, -1, -1, -1, -1, -1, -1, 126}},
	{UINT64_MAX >> 1,   9, {-1, -1, -1, -1, -1, -1, -1, -1, 127}},
	{UINT64_C(1) << 63, 10,{-127, -128, -128, -128, -128, -128, -128, -128, -128, 0}},
	{128, 2, {-127, 0}},
	{8192, 2, {-64, 0}}
};

const struct {
	int64_t n;
	uint8_t data_size;
	uint8_t data[10];
} be_signed_tests[] = {
	{ INT64_MAX,        10,{-128, -1, -1, -1, -1, -1, -1, -1, -1, 127}},
	{ INT64_MIN,        10,{-127, -128, -128, -128, -128, -128, -128, -128, -128, 0}},
	{ INT64_MAX - 1,    10,{-128, -1, -1, -1, -1, -1, -1, -1, -1, 126}},
	{ INT64_MAX >> 1,   9, {-65, -1, -1, -1, -1, -1, -1, -1, 127}},
	{ INT64_MIN + 1,    10,{-127, -128, -128, -128, -128, -128, -128, -128, -128, 1}},
	{ INT64_C(1) << 62, 10,{-128, -64, -128, -128, -128, -128, -128, -128, -128, 0}}
};

#define len(arr) (sizeof(arr) / sizeof(arr[0]))

#define NUM_TEST (1<<(3 * 7))

#define EMPTY_BYTE 0b01010101
static bool is_empty(const uint8_t *ptr, size_t size) {
	for (size_t i = 0; i < size; i++) if (ptr[i] != EMPTY_BYTE) return false;
	return true;
}

int main() {
	uint8_t buff[10];

	for (uint64_t i = 0; i < NUM_TEST; i++) {
		uint64_t ret;
		check("Varint encoding doesn't falsely overflow", leuv_enc(buff, buff + 10, i));
		check("Varint decoding doesn't falsely underflow", leuv_dec(buff, buff + 10, &ret));
		check("Varint encoding produces the same result", ret == i);
	}

	for (uint64_t i = 0; i < len(le_unsigned_tests); i++) {
		uint64_t ret;
		memset(buff, EMPTY_BYTE, 10);
		check("Varint encoding doesn't falsely overflow", leuv_enc(buff, buff + 10, le_unsigned_tests[i].n));
		check("Varint decoding doesn't falsely underflow", leuv_dec(buff, buff + 10, &ret));
		check("Varint encoding produces the same result", ret == le_unsigned_tests[i].n);
		check("Varint encoding wrote correct bytes",
			memcmp(buff, le_unsigned_tests[i].data, le_unsigned_tests[i].data_size) == 0);
		check("Varint encoding didn't write outside bounds",
			is_empty(buff + le_unsigned_tests[i].data_size, 10 - le_unsigned_tests[i].data_size));
		check("Varint reports correct number of bytes",
			leuv_size(le_unsigned_tests[i].n) == le_unsigned_tests[i].data_size);
	}

	for (int64_t i = -NUM_TEST / 2; i < NUM_TEST / 2; i++) {
		int64_t ret;
		check("Varint encoding doesn't falsely overflow", leiv_enc(buff, buff + 10, i));
		check("Varint decoding doesn't falsely underflow", leiv_dec(buff, buff + 10, &ret));
		check("Varint encoding produces the same result", ret == i);
	}

	for (uint64_t i = 0; i < len(le_signed_tests); i++) {
		int64_t ret;
		memset(buff, 0b01010101, 10);
		check("Varint encoding doesn't falsely overflow", leiv_enc(buff, buff + 10, le_signed_tests[i].n));
		check("Varint decoding doesn't falsely underflow", leiv_dec(buff, buff + 10, &ret));
		check("Varint encoding produces the same result", ret == le_signed_tests[i].n);
		check("Varint encoding wrote correct bytes",
			memcmp(buff, le_signed_tests[i].data, le_signed_tests[i].data_size) == 0);
		check("Varint encoding didn't write outside bounds",
			is_empty(buff + le_signed_tests[i].data_size, 10 - le_signed_tests[i].data_size));
		check("Varint reports correct number of bytes",
			leiv_size(le_signed_tests[i].n) == le_signed_tests[i].data_size);
	}

	for (uint64_t i = 0; i < NUM_TEST; i++) {
		uint64_t ret;
		check("Varint encoding doesn't falsely overflow", beuv_enc(buff, buff + 10, i));
		check("Varint decoding doesn't falsely underflow", beuv_dec(buff, buff + 10, &ret));
		check("Varint encoding produces the same result", ret == i);
	}

	for (uint64_t i = 0; i < len(be_unsigned_tests); i++) {
		uint64_t ret;
		memset(buff, 0b01010101, 10);
		check("Varint encoding doesn't falsely overflow", beuv_enc(buff, buff + 10, be_unsigned_tests[i].n));
		check("Varint decoding doesn't falsely underflow", beuv_dec(buff, buff + 10, &ret));
		check("Varint encoding produces the same result", ret == be_unsigned_tests[i].n);
		check("Varint encoding wrote correct bytes",
			memcmp(buff, be_unsigned_tests[i].data, be_unsigned_tests[i].data_size) == 0);
		check("Varint encoding didn't write outside bounds",
			is_empty(buff + be_unsigned_tests[i].data_size, 10 - be_unsigned_tests[i].data_size));
		check("Varint reports correct number of bytes",
			beuv_size(be_unsigned_tests[i].n) == be_unsigned_tests[i].data_size);
	}

	for (int64_t i = -NUM_TEST / 2; i < NUM_TEST / 2; i++) {
		int64_t ret;
		check("Varint encoding doesn't falsely overflow", beiv_enc(buff, buff + 10, i));
		check("Varint decoding doesn't falsely underflow", beiv_dec(buff, buff + 10, &ret));
		check("Varint encoding produces the same result", ret == i);
	}

	for (uint64_t i = 0; i < len(be_signed_tests); i++) {
		int64_t ret;
		memset(buff, 0b01010101, 10);
		check("Varint encoding doesn't falsely overflow", beiv_enc(buff, buff + 10, be_signed_tests[i].n));
		check("Varint decoding doesn't falsely underflow", beiv_dec(buff, buff + 10, &ret));
		check("Varint encoding produces the same result", ret == be_signed_tests[i].n);
		check("Varint encoding wrote correct bytes",
			memcmp(buff, be_signed_tests[i].data, be_signed_tests[i].data_size) == 0);
		check("Varint encoding didn't write outside bounds",
			is_empty(buff + be_signed_tests[i].data_size, 10 - be_signed_tests[i].data_size));
		check("Varint reports correct number of bytes",
			beiv_size(be_signed_tests[i].n) == be_signed_tests[i].data_size);
	}
}
