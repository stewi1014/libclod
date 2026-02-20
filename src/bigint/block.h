#ifndef LIBCLOD_BLOCK_H
#define LIBCLOD_BLOCK_H

#include <clod/lib.h>
#include <stdint.h>
#include <stddef.h>

/// Unit of storage for bigints
typedef uint64_t block;
/// Maximum value a block can represent
#define BLOCK_MAX UINT64_MAX
/// Size of an array element in bytes
#define BLOCK_SIZE sizeof(block)
/// Size of an array element in bits
#define BLOCK_BITS UINT64_WIDTH
/// Half the size of a block in bits
#define BLOCK_HALF_BITS (UINT64_WIDTH / 2u)
/// Mask of the lower half of a block
#define BLOCK_HALF_MASK 0x00000000FFFFFFFF

block add(block a, block b, block c, block *carry_out);
block sub(block a, block b, block c, block *borrow_out);
block mul(block a, block b, block *high_out);
block div(block a_hi, block a_lo, block b, block *remainder);

void add_blocks(
	const block *a, size_t a_len,
	const block *b, size_t b_len,
	block carry,
	block *out, size_t out_len, block *carry_out
);

void sub_blocks(
	const block *a, size_t a_len,
	const block *b, size_t b_len,
	block borrow,
	block *out, size_t out_len, block *borrow_out
);

void mul_blocks(
	const block *a, size_t a_len,
	const block *b, size_t b_len,
	block *out, size_t out_len, bool *out_overflowed
);

void div_blocks(
	const block *a, size_t a_len,
	const block *b, size_t b_len,
	block *quot, size_t quot_len, bool *quot_overflowed,
	block *rem, size_t rem_len, bool *rem_overflowed,
	bool ceiling
);

#endif
