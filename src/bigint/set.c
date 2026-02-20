#include "bigint_impl.h"
#include <stdbit.h>

uint32_t clod_bigint_set_uint64(clod_bigint *bigint, const uint64_t val) {
	uint32_t bits = stdc_bit_width(val);
	if (grow(bigint, bits) < ARRAY_LEN(bits)) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return bits;
	}

	if (bits > 0) ARRAY(bigint)[0] = val;
	if (bits > 32) ARRAY(bigint)[1] = val >> 32;

	bigint->bits = bits;
	bigint->sign = false;
	bigint->result = CLOD_BIGINT_OK;
	return 0;
}
uint32_t clod_bigint_set_int64(clod_bigint *bigint, const int64_t val) {
	uint64_t magnitude = val < 0 ? -(uint64_t)val : (uint64_t)val;
	uint32_t bits = stdc_bit_width(magnitude);
	if (grow(bigint, bits) < ARRAY_LEN(bits)) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return bits;
	}

	if (bits > 0) ARRAY(bigint)[0] = magnitude;
	if (bits > 32) ARRAY(bigint)[1] = magnitude >> 32;

	bigint->bits = bits;
	bigint->sign = val < 0;
	bigint->result = CLOD_BIGINT_OK;
	return 0;
}
