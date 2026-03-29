#include "debug.h"
#include <stdint.h>
#include <string.h>
#include "serialise/integer_be.h"
#include "serialise/integer_le.h"

const struct {
	uint64_t n;
	size_t data_size;
	char data[10];
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
	size_t data_size;
	char data[10];
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
	size_t data_size;
	char data[10];
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
	size_t data_size;
	char data[10];
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

int serialise_integer(int, char[]) {
	uint8_t buff[10];

	for (uint64_t i = 0; i < NUM_TEST; i++) {
		uint64_t ret;
		assert_fatal(CLOD_TEST, leuv_enc(buff, buff + 10, i), "Varint encoding doesn't falsely overflow");
		assert_fatal(CLOD_TEST, leuv_dec(buff, buff + 10, &ret), "Varint decoding doesn't falsely underflow");
		assert_fatal(CLOD_TEST, ret == i, "Varint encoding produces the same result");
	}

	for (uint64_t i = 0; i < len(le_unsigned_tests); i++) {
		uint64_t ret;
		memset(buff, EMPTY_BYTE, 10);
		assert_fatal(CLOD_TEST, leuv_enc(buff, buff + 10, le_unsigned_tests[i].n), "Varint encoding doesn't falsely overflow");
		assert_fatal(CLOD_TEST, leuv_dec(buff, buff + 10, &ret), "Varint decoding doesn't falsely underflow");
		assert_fatal(CLOD_TEST, ret == le_unsigned_tests[i].n, "Varint encoding produces the same result");
		assert_fatal(CLOD_TEST, memcmp(buff, le_unsigned_tests[i].data, le_unsigned_tests[i].data_size) == 0, "Varint encoding wrote correct bytes");
		assert_fatal(CLOD_TEST, is_empty(buff + le_unsigned_tests[i].data_size, 10 - le_unsigned_tests[i].data_size), "Varint encoding didn't write outside bounds");
		assert_fatal(CLOD_TEST, leuv_size(le_unsigned_tests[i].n) == le_unsigned_tests[i].data_size, "Varint reports correct number of bytes");
	}

	for (int64_t i = -NUM_TEST / 2; i < NUM_TEST / 2; i++) {
		int64_t ret;
		assert_fatal(CLOD_TEST, leiv_enc(buff, buff + 10, i), "Varint encoding doesn't falsely overflow");
		assert_fatal(CLOD_TEST, leiv_dec(buff, buff + 10, &ret), "Varint decoding doesn't falsely underflow");
		assert_fatal(CLOD_TEST, ret == i, "Varint encoding produces the same result. Encoded %i64, decoded %i64", i, ret);
	}

	for (uint64_t i = 0; i < len(le_signed_tests); i++) {
		int64_t ret;
		memset(buff, 0b01010101, 10);
		assert_fatal(CLOD_TEST, leiv_enc(buff, buff + 10, le_signed_tests[i].n), "Varint encoding doesn't falsely overflow");
		assert_fatal(CLOD_TEST, leiv_dec(buff, buff + 10, &ret), "Varint decoding doesn't falsely underflow");
		assert_fatal(CLOD_TEST, ret == le_signed_tests[i].n, "Varint encoding produces the same result");
		assert_fatal(CLOD_TEST, memcmp(buff, le_signed_tests[i].data, le_signed_tests[i].data_size) == 0, "Varint encoding wrote correct bytes");
		assert_fatal(CLOD_TEST, is_empty(buff + le_signed_tests[i].data_size, 10 - le_signed_tests[i].data_size), "Varint encoding didn't write outside bounds");
		assert_fatal(CLOD_TEST, leiv_size(le_signed_tests[i].n) == le_signed_tests[i].data_size, "Varint reports correct number of bytes");
	}

	for (uint64_t i = 0; i < NUM_TEST; i++) {
		uint64_t ret;
		assert_fatal(CLOD_TEST, beuv_enc(buff, buff + 10, i), "Varint encoding doesn't falsely overflow");
		assert_fatal(CLOD_TEST, beuv_dec(buff, buff + 10, &ret), "Varint decoding doesn't falsely underflow");
		assert_fatal(CLOD_TEST, ret == i, "Varint encoding produces the same result");
	}

	for (uint64_t i = 0; i < len(be_unsigned_tests); i++) {
		uint64_t ret;
		memset(buff, 0b01010101, 10);
		assert_fatal(CLOD_TEST, beuv_enc(buff, buff + 10, be_unsigned_tests[i].n), "Varint encoding doesn't falsely overflow");
		assert_fatal(CLOD_TEST, beuv_dec(buff, buff + 10, &ret), "Varint decoding doesn't falsely underflow");
		assert_fatal(CLOD_TEST, ret == be_unsigned_tests[i].n, "Varint encoding produces the same result");
		assert_fatal(CLOD_TEST, memcmp(buff, be_unsigned_tests[i].data, be_unsigned_tests[i].data_size) == 0, "Varint encoding wrote correct bytes");
		assert_fatal(CLOD_TEST, is_empty(buff + be_unsigned_tests[i].data_size, 10 - be_unsigned_tests[i].data_size), "Varint encoding didn't write outside bounds");
		assert_fatal(CLOD_TEST, beuv_size(be_unsigned_tests[i].n) == be_unsigned_tests[i].data_size, "Varint reports correct number of bytes");
	}

	for (int64_t i = -NUM_TEST / 2; i < NUM_TEST / 2; i++) {
		int64_t ret;
		assert_fatal(CLOD_TEST, beiv_enc(buff, buff + 10, i), "Varint encoding doesn't falsely overflow");
		assert_fatal(CLOD_TEST, beiv_dec(buff, buff + 10, &ret), "Varint decoding doesn't falsely underflow");
		assert_fatal(CLOD_TEST, ret == i, "Varint encoding produces the same result");
	}

	for (uint64_t i = 0; i < len(be_signed_tests); i++) {
		int64_t ret;
		memset(buff, 0b01010101, 10);
		assert_fatal(CLOD_TEST, beiv_enc(buff, buff + 10, be_signed_tests[i].n), "Varint encoding doesn't falsely overflow");
		assert_fatal(CLOD_TEST, beiv_dec(buff, buff + 10, &ret), "Varint decoding doesn't falsely underflow");
		assert_fatal(CLOD_TEST, ret == be_signed_tests[i].n, "Varint encoding produces the same result");
		assert_fatal(CLOD_TEST, memcmp(buff, be_signed_tests[i].data, be_signed_tests[i].data_size) == 0, "Varint encoding wrote correct bytes");
		assert_fatal(CLOD_TEST, is_empty(buff + be_signed_tests[i].data_size, 10 - be_signed_tests[i].data_size), "Varint encoding didn't write outside bounds");
		assert_fatal(CLOD_TEST, beiv_size(be_signed_tests[i].n) == be_signed_tests[i].data_size, "Varint reports correct number of bytes");
	}

	return 0;
}
