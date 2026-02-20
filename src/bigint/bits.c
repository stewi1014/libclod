#include "bigint_impl.h"
#include <string.h>

bool clod_bigint_bit_get(const clod_bigint *bigint, const uint32_t index) {
	if (index >= bigint->bits) return false;
	return ARRAY(bigint)[INDEX(index)] & MASK(index);
}

uint32_t clod_bigint_bit_set(clod_bigint *bigint, const uint32_t index) {
	if (index == UINT32_MAX) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return UINT32_MAX;
	}
	if (grow_clear(bigint, ARRAY_LEN(index + 1)) < ARRAY_LEN(index + 1)) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return index + 1;
	}
	bigint->bits = max(bigint->bits, index + 1);
	ARRAY(bigint)[INDEX(index)] |= MASK(index);
	return 0;
}

void clod_bigint_bit_unset(clod_bigint *bigint, const uint32_t index) {
	if (index >= bigint->bits) return;
	ARRAY(bigint)[INDEX(index)] &=~ MASK(index);
	normalise(bigint);
}
