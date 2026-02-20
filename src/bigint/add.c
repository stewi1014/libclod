#include "bigint_impl.h"

static uint32_t add_(clod_bigint *bigint, const clod_bigint *val) {
	if (val->bits == 0) return 0;
	uint32_t cap = grow_clear(bigint, ARRAY_LEN(max(bigint->bits, val->bits) + 1));
	if (cap == 0) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return max(bigint->bits, val->bits);
	}

	uint64_t *array = ARRAY(bigint);
	uint64_t *val_array = ARRAY(val);

	block carry = 0;
	uint32_t i = 0;
	while (i < cap && i < ARRAY_LEN(val->bits)) {
		array[i] = add_carry(array[i], val_array[i], carry, &carry);
		i++;
	}
	while (i < cap && carry) {
		array[i] = add_carry(array[i], 0, carry, &carry);
		i++;
	}
	bigint->bits = (i + 1) * BLOCK_BITS;
	normalise(bigint);
	if (i < ARRAY_LEN(val->bits)) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return val->bits + 1;
	}
	if (carry) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return bigint->bits + 1;
	}
	return 0;
}

static uint32_t sub_(clod_bigint *bigint, const clod_bigint *val) {
	if (val->bits == 0) return 0;
	uint32_t cap = grow_clear(bigint, ARRAY_LEN(max(bigint->bits, val->bits)));
	if (cap == 0) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return max(bigint->bits, val->bits);
	}

	uint64_t *array = ARRAY(bigint);
	uint64_t *val_array = ARRAY(val);
	uint64_t carry = 0;
	uint32_t i = 0;

	int sign = magnitude_cmp(bigint, val, nullptr);
	if (sign < 0) {
		// val > bigint
		while (i < cap && i < ARRAY_LEN(val->bits)) {
			array[i] = sub_carry(val_array[i], array[i], carry, &carry);
			i++;
		}
		bigint->sign = !bigint->sign;
	}
	if (sign > 0) {
		// bigint > val
		while (i < cap && i < ARRAY_LEN(val->bits)) {
			array[i] = sub_carry(array[i], val_array[i], carry, &carry);
			i++;
		}
		while (i < cap && carry) {
			array[i] = sub_carry(array[i], 0, carry, &carry);
			i++;
		}
	}
	if (sign == 0) {
		bigint->bits = 0;
		bigint->sign = 0;
		return 0;
	}

	bigint->bits = i * 32 + 32;
	normalise(bigint);

	if (carry) {
		bigint->result = CLOD_BIGINT_OVERFLOW;
		return max(bigint->bits, val->bits);
	}
	return 0;
}

uint32_t clod_bigint_add(clod_bigint *bigint, const clod_bigint *val) {
	if (bigint->sign == val->sign)
		return add_(bigint, val);
	return sub_(bigint, val);
}

uint32_t clod_bigint_sub(clod_bigint *bigint, const clod_bigint *val) {
	if (bigint->sign != val->sign)
		return add_(bigint, val);
	return sub_(bigint, val);
}
