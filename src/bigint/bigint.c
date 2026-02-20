#include "bigint_impl.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbit.h>

void *default_realloc(void *ptr, const size_t size, void*) { return realloc(ptr, size); }

clod_bigint *clod_bigint_dynamic(void*(*realloc_func)(void *ptr, size_t size, void* user), void *user) {
	if (!realloc_func) realloc_func = default_realloc;

	struct clod_bigint_dynamic *dyn = realloc_func(nullptr, sizeof(struct clod_bigint_dynamic), user);
	if (!dyn) return nullptr;

	dyn->bigint.bits = 0;
	dyn->bigint.cap = 0;
	dyn->bigint.sign = 0;
	dyn->bigint.dynamic = true;
	dyn->bigint.result = CLOD_BIGINT_OK;
	dyn->array = nullptr;
	dyn->realloc_func = realloc_func;
	dyn->user = user;
	return &dyn->bigint;
}
void clod_bigint_free(clod_bigint *bigint) {
	struct clod_bigint_dynamic *dyn = DYNAMIC(bigint);
	if (!dyn) return;

	dyn->realloc_func(dyn->array, 0, dyn->user);
	dyn->realloc_func(dyn, 0, dyn->user);
}
clod_bigint *clod_bigint_static(void *memory, const uint32_t bits) {
	if (bits > CLOD_BIGINT_MAX || (uintptr_t)memory % alignof(clod_bigint))
		return nullptr;

	struct clod_bigint_static *stat = memory;
	stat->bigint.bits = 0;
	stat->bigint.cap = ARRAY_LEN(bits);
	stat->bigint.sign = 0;
	stat->bigint.dynamic = false;
	stat->bigint.result = CLOD_BIGINT_OK;
	memset(stat->array, 0, ARRAY_LEN(bits) * BLOCK_SIZE);
	return &stat->bigint;
}
enum clod_bigint_result clod_bigint_result(const clod_bigint *bigint) {
	return bigint->result;
}
bool clod_bigint_sign(const clod_bigint *bigint) {
	return bigint->sign;
}
void clod_bigint_set_sign(clod_bigint *bigint, const bool sign) {
	if (bigint->bits == 0) return;
	bigint->sign = sign;
}
uint32_t clod_bigint_bits(const clod_bigint *bigint) {
	return bigint->bits;
}
int clod_bigint_cmp(const clod_bigint *bigint1, const clod_bigint *bigint2) {
	if (bigint1 == bigint2) return 0;
	if (bigint1->sign != bigint2->sign) return bigint1->sign - bigint2->sign;
	return magnitude_cmp(bigint1, bigint2, nullptr);
}

int magnitude_cmp(const clod_bigint *bigint1, const clod_bigint *bigint2, uint32_t *index) {
	if (bigint1->bits > bigint2->bits) {
		if (index) *index = ARRAY_LEN(max(bigint1->bits, bigint2->bits)) - 1;
		return 1;
	}
	if (bigint1->bits < bigint2->bits) {
		if (index) *index = ARRAY_LEN(max(bigint1->bits, bigint2->bits)) - 1;
		return -1;
	}

	block *array1 = ARRAY(bigint1);
	block *array2 = ARRAY(bigint2);
	uint32_t i = ARRAY_LEN(bigint1->bits) - 1;
	while (i > 0 && array1[i] == array2[i]) i++;

	if (index) *index = i;
	if (array1[i] > array2[i]) return 1;
	if (array1[i] < array2[i]) return -1;
	return 0;
}
bool normalise(clod_bigint *bigint) {
	if (bigint->bits == 0) {
		bigint->sign = false;
		return true;
	}

	uint64_t *array = ARRAY(bigint);
	uint32_t index = ARRAY_LEN(bigint->bits);

	while (index > 0 && array[index - 1] == 0) index--;

	if (index == 0) {
		bigint->bits = 0;
		bigint->sign = false;
		return false;
	}

	uint32_t old_bits = bigint->bits;
	bigint->bits = (index - 1) * BLOCK_BITS + stdc_bit_width(array[index - 1]);
	return bigint->bits == old_bits;
}
uint32_t grow(clod_bigint *bigint, const uint32_t cap) {
	if (bigint->cap >= cap) return bigint->cap;
	if (!bigint->dynamic) return bigint->cap;

	struct clod_bigint_dynamic *dyn = DYNAMIC(bigint);
	uint32_t new_cap = max((uint32_t)(bigint->cap << 1) - (bigint->cap >> 1), cap);
	uint64_t *new_array = dyn->realloc_func(dyn->array, new_cap * BLOCK_SIZE, dyn->user);
	if (!new_array) return bigint->cap;
	dyn->array = new_array;

	bigint->cap = new_cap;
	return bigint->cap;
}
uint32_t grow_clear(clod_bigint *bigint, const uint32_t cap) {
	uint32_t new_cap = grow(bigint, cap);
	if (ARRAY_LEN(bigint->bits) < min(new_cap, cap)) {
		memset(
			ARRAY(bigint) + ARRAY_LEN(bigint->bits),
			0,
			(min(cap, new_cap) - ARRAY_LEN(bigint->bits)) * BLOCK_SIZE
		);
	}
	return new_cap;
}
