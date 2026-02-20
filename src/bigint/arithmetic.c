#include "block.h"
#include <assert.h>

static bool buffers_alias(const block *a, const size_t a_len, const block *b, const size_t b_len) {
	if (a + a_len <= b) return false;
	if (b + b_len <= a) return false;
	return true;
}

block add(const block a, const block b, const block c, block *carry_out) {
	block result = a, carry = 0;

	result += b;
	carry += result < b;

	result += c;
	carry += result < c;

	*carry_out = carry;
	return result;
}

void add_blocks(
	const block *a, const size_t a_len,
	const block *b, const size_t b_len,
	block carry,
	block *out, const size_t out_len, block *carry_out
) {
	assert(!buffers_alias(a, a_len, out, out_len) || a >= out);
	assert(!buffers_alias(b, b_len, out, out_len) || b >= out);

	for (size_t i = 0; i < out_len; i++)
		out[i] = add(i < a_len ? a[i] : 0, i < b_len ? b[i] : 0, carry, &carry);
	if (carry_out) *carry_out = carry;
}

block sub(const block a, const block b, const block c, block *borrow_out) {
	block result = a, borrow = 0;

	borrow += result < b;
	result -= b;

	borrow += result < c;
	result -= c;

	*borrow_out = borrow;
	return result;
}

void sub_blocks(
	const block *a, const size_t a_len,
	const block *b, const size_t b_len,
	block borrow,
	block *out, const size_t out_len, block *borrow_out
) {
	assert(!buffers_alias(a, a_len, out, out_len) || a >= out);
	assert(!buffers_alias(b, b_len, out, out_len) || b >= out);

	for (size_t i = 0; i < out_len; i++)
		out[i] = sub(i < a_len ? a[i] : 0, i < b_len ? b[i] : 0, borrow, &borrow);
	if (borrow_out) *borrow_out = borrow;
}

block mul(const block a, const block b, block *high_out) {
	block a_lo = a & BLOCK_HALF_MASK;
	block b_lo = b & BLOCK_HALF_MASK;
	block a_hi = a >> BLOCK_HALF_BITS;
	block b_hi = b >> BLOCK_HALF_BITS;

	block ab_lo = a_lo * b_lo;
	block ab_mid1 = a_hi * b_lo;
	block ab_mid2 = a_lo * b_hi;
	block ab_hi = a_hi * b_hi;

	block carry;
	block lo = add(ab_lo, ab_mid1 << BLOCK_HALF_BITS, ab_mid2 << BLOCK_HALF_BITS, &carry);
	*high_out = ab_hi + (ab_mid1 >> BLOCK_HALF_BITS) + (ab_mid2 >> BLOCK_HALF_BITS) + carry;
	return lo;
}

void mul_blocks(
	const block *a, const size_t a_len,
	const block *b, const size_t b_len,
	block *out, const size_t out_len, bool *out_overflowed
) {
	assert(!buffers_alias(a, a_len, out, out_len));
	assert(!buffers_alias(b, b_len, out, out_len));

	for (size_t i = 0; i < out_len; i++) out[i] = 0;

	bool overflowed = false;
	for (size_t a_i = 0; a_i < a_len && a_i < out_len; a_i++) {
		for (size_t b_i = 0; b_i < b_len && a_i + b_i < out_len; b_i++) {
			block res[2];
			res[0] = mul(a[a_i], b[b_i], &res[1]);

			block carry;
			add_blocks(
				out + a_i + b_i, out_len - (a_i + b_i),
				res, 2, 0,
				out + a_i + b_i, out_len - (a_i + b_i),
				&carry
			);
			if (carry) overflowed = true;
		}
	}
	if (out_overflowed) *out_overflowed = overflowed;
}

block div(block a_hi, block a_lo, block b, block *remainder) {
	*remainder = a % b;
	return a / b;
}

void div_blocks(
	const block *a, const size_t a_len,
	const block *b, const size_t b_len,
	block *quot, const size_t quot_len, bool *quot_overflowed,
	block *rem, const size_t rem_len, bool *rem_overflowed,
	bool ceiling
) {

}