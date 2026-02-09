#include <assert.h>
#include <string.h>
#include <clod/btree.h>
#include <clod/endian_big.h>

/**
 * # Root Node
 * | Offset | Size | Name
 * |--------|------|-------------
 * | 0      | 2    | Flags
 * | 2      | 2    | N Elems
 * | 8      | 8    | Generation
 * | 16     | 8    | Unused
 * | 24     | 8    | First Leaf Node
 * | 32     | 8    | Page Count - 1
 * | 40     | 8    | First Deleted Node
 * | 96     | 4000 | Internal or Leaf payload
 *
 * # Internal Node
 * | Offset | Size | Name
 * |--------|------|------------
 * | 0      | 2    | Flags
 * | 2      | 2    | N Elems
 * | 8      | 8    | Generation
 * | 16     | 8    | Parent Node
 * | 96     | 4000 | Internal payload
 *
 * # Leaf Node
 * | Offset | Size | Name
 * |--------|------|------------
 * | 0      | 2    | Flags
 * | 2      | 2    | N Elems
 * | 8      | 8    | Generation
 * | 16     | 8    | Parent Node
 * | 24     | 8    | Next Leaf Node
 * | 32     | 8    | Prev Leaf Node
 * | 96     | 4000 | Leaf payload
 *
 * # Internal payload
 * | Offset | Size | Name
 * |--------|------|-----------
 * | 96     | 8    | Node 1 offset
 * | 104    | 1    | Key 1 Size
 * | 105    | *    | Key 1
 * ...
 * | *      | 8    | Node N offset
 * | *      | 1    | Key N Size
 * | *      | *    | Key N
 * | *      | 8    | Node N+1 offset
 *
 * # Leaf payload
 * | Offset | Size | Name
 * |--------|------|------------
 * | 96     | 1    | Key 1 Size
 * | 97     | *    | Key 1
 * | *      | 1    | Value 1 Size
 * | *      | *    | Value 1
 * ...
 * | *      | 1    | Key N Size
 * | *      | *    | Key N
 * | *      | 1    | Value N Size
 * | *      | *    | Value N
 *
 * # Deleted Page
 * | Offset | Size | Name
 * |--------|------|------------
 * | 0      | 8    | Next Deleted Page
 */

/// Size of a page
#define PAGE_SIZE (1 << 12)

/// Maximum theoretical depth of the tree.
/// Internal nodes have a minimum of 16 branches.
/// 16^13 * 4096 exhausts the 64bit address space,
/// so a depth of 13 is the theoretical maximum.
#define MAX_DEPTH 13

/// Maximum number of spins before considering the owner dead and/or the btree malformed.
#define MAX_SPINS 100000

#define FLAG_INTERNAL UINT16_C(0b00000000'00000001)
#define FLAG_NOT_ROOT UINT16_C(0b00000000'00000010)
#define FLAG_WRLOCK   UINT16_C(0b10000000'00000000)

/// Pointer to somewhere within a node.
typedef uint16_t node_ptr;

static int compare(
	const uint8_t *a, uint8_t a_size,
	const uint8_t *b, uint8_t b_size
) {
	const uint8_t min_size = a_size < b_size ? a_size : b_size;
	for (uint8_t i = 0; i < min_size; i++) {
		if (a[i] < b[i]) return -1;
		if (a[i] > b[i]) return 1;
	}
	return a_size - b_size;
}

static uint64_t atomic_beu64_load(const uint8_t *ptr) {
	const uint64_t val = __atomic_load_n((const uint64_t*)ptr, __ATOMIC_SEQ_CST);
	return beu64_dec((uint8_t*)&val);
}
static void atomic_beu64_store(const uint8_t *ptr, uint64_t val) {
	beu64_enc((uint8_t*)&val, val);
	__atomic_store_n((uint64_t*)ptr, val, __ATOMIC_SEQ_CST);
}
static bool atomic_beu64_cas(const uint8_t *ptr, uint64_t *expected, uint64_t desired) {
	uint64_t expected_be, desired_be;
	beu64_enc((uint8_t*)&expected_be, *expected);
	beu64_enc((uint8_t*)&desired_be, desired);
	const bool ok = __atomic_compare_exchange_n(
		(uint64_t*)ptr, &expected_be, desired_be,
		false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
	);
	if (!ok) *expected = beu64_dec((uint8_t*)&expected_be);
	return ok;
}

bool generation_lock(const uint8_t *ptr, uint64_t *generation_out) {
	uint64_t generation = atomic_beu64_load(ptr);
	for (int i = 0; i < MAX_SPINS; i++) {
		generation &=~ UINT64_C(1);
		if (atomic_beu64_cas(ptr, &generation, generation + 1)) {
			*generation_out = generation + 1;
			return true;
		}
	}
	return false;
}
bool generation_unlock(const uint8_t *ptr, uint64_t generation) {
	if ((generation & 1) == 0) return false;
	return atomic_beu64_cas(ptr, &generation, generation + 1);
}
bool generation_wait(const uint8_t *ptr, uint64_t *generation_out) {
	uint64_t generation = atomic_beu64_load(ptr);
	for (int i = 0; i < MAX_SPINS; i++) {
		if ((generation & 1) == 0) {
			*generation_out = generation;
			return true;
		}
		generation = atomic_beu64_load(ptr);
	}
	return false;
}
bool generation_same(const uint8_t *ptr, uint64_t generation) {
	const uint64_t current = atomic_beu64_load(ptr);
	 return generation == current;
}

struct internal_iter {
	node_ptr key;
	uint8_t key_size;
	node_ptr left_branch;
	node_ptr right_branch;
	uint16_t index;
};
enum clod_btree_result internal_iter(const uint8_t *node, struct internal_iter *iter) {
	assert(beu8_dec(node) & FLAG_INTERNAL);
	const uint16_t count = beu16_dec(node + 2);

	if (iter->key == 0) {
		if (count == 0) {
			return CLOD_BTREE_NOT_EXIST;
		}

		iter->key = 16 + 8 + 1;
		iter->key_size = beu8_dec(node + 16 + 8);
		iter->index = 0;
		iter->left_branch = 16;
		iter->right_branch = 16 + 8 + 1 + iter->key_size;
	} else {
		if (iter->index >= count) {
			iter->key = 0;
			return CLOD_BTREE_NOT_EXIST;
		}
		const node_ptr elem = iter->key + iter->key_size;
		iter->key = elem + 8 + 1;
		if (elem + 8 + 1 > PAGE_SIZE) return CLOD_BTREE_MALFORMED;
		iter->key_size = beu8_dec(node + elem + 8);
		iter->index++;
		iter->left_branch = elem;
		iter->right_branch = elem + 8 + 1 + iter->key_size;
		if (iter->right_branch + 8 > PAGE_SIZE) return CLOD_BTREE_MALFORMED;
	}

	return CLOD_BTREE_OK;
}
struct leaf_iter {
	node_ptr key;
	uint8_t key_size;
	uint8_t value_size;
	node_ptr value;
	uint16_t index;
};
enum clod_btree_result leaf_iter(const uint8_t *node, struct leaf_iter *iter) {
	assert(!(beu8_dec(node) & FLAG_INTERNAL));
	const uint16_t count = beu16_dec(node + 2);

	if (iter->key == 0) {
		if (count == 0) {
			return CLOD_BTREE_NOT_EXIST;
		}
		iter->key = 16 + 1;
		iter->key_size = beu8_dec(node + 16);
		iter->index = 0;
		iter->value = 16 + 1 + iter->key_size + 1;
		iter->value_size = beu8_dec(node + 16 + 1 + iter->key_size);
	} else {
		if (iter->index >= count) {
			iter->key = 0;
			return CLOD_BTREE_NOT_EXIST;
		}
		const node_ptr elem = iter->key + iter->key_size + 1 + iter->value_size;
		iter->key = elem + 1;
		if (elem + 1 > PAGE_SIZE) return CLOD_BTREE_MALFORMED;
		iter->key_size = beu8_dec(node + elem);
		iter->index++;
		iter->value = elem + 1 + iter->key_size + 1;
		if (elem + 1 + iter->key_size + 1 > PAGE_SIZE) return CLOD_BTREE_MALFORMED;
		iter->value_size = beu8_dec(node + elem + 1 + iter->key_size);
		if (iter->value + iter->value_size > PAGE_SIZE) return CLOD_BTREE_MALFORMED;
	}

	return CLOD_BTREE_OK;
}

enum clod_btree_result branch_find(
	const uint8_t *node, struct internal_iter *iter,
	const uint8_t *key, uint8_t key_size,
	node_ptr *branch_out
) {
	enum clod_btree_result res;
	while ((res = internal_iter(node, iter)) == CLOD_BTREE_OK) {
		const int cmp = compare(key, key_size, node + iter->key, iter->key_size);
		if (cmp <= 0) {
			*branch_out = iter->left_branch;
			return CLOD_BTREE_OK;
		}
	}
	*branch_out = iter->right_branch;
	return res;
}

enum clod_btree_result leaf_find(
	const uint8_t *node, struct leaf_iter *iter,
	const uint8_t *key, uint8_t key_size
) {
	enum clod_btree_result res;
	while ((res = leaf_iter(node, iter)) == CLOD_BTREE_OK) {
		const int cmp = compare(key, key_size, node + iter->key, iter->key_size);
		if (cmp < 0) return CLOD_BTREE_NOT_EXIST;
		if (cmp == 0) return CLOD_BTREE_OK;
	}
	return res;
}

enum clod_btree_result internal_traverse(
	const uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size,
	uint8_t **leaf_out, uint64_t *generation_out
)

enum clod_btree_result btree_get(
	const uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *node,
	const uint8_t *key, uint8_t key_size,
	uint8_t *value, uint8_t *value_size
) {
	const uint16_t flags = beu16_dec(node);
	enum clod_btree_result res;

	if (flags & FLAG_INTERNAL) {
		struct internal_iter iter;
		uint64_t branch;
		while ((res = internal_iter(node, &iter)) == CLOD_BTREE_OK) {
			const int cmp = compare(key, key_size, node + iter.key, iter.key_size);
			if (cmp <= 0) {
				branch = beu64_dec(node + iter.left_branch);
				branch = iter.left_branch;
				goto found_branch;
			}
		}
		if (res != CLOD_BTREE_OK)
			return res;
		branch = beu64_dec(node + iter.right_branch);
	found_branch:

		if ((uintptr_t)(btree_end - btree) < branch + PAGE_SIZE)
			return CLOD_BTREE_MALFORMED;

		uint64_t generation;
		if (!generation_wait(btree + branch + 8, &generation))
			return CLOD_BTREE_BUSY;

		res = btree_get(btree, btree_end, btree + branch, key, key_size, value, value_size);

		if (!generation_same(btree + branch + 8, generation))
			return CLOD_BTREE_BUSY;
		return res;
	}

	struct leaf_iter iter;
	auto res = leaf_find(node, &iter, key, key_size);
	if (res != CLOD_BTREE_OK)
		return res;

	if (value) {
		size_t size = iter.value_size;
		if (value_size && *value_size < size) size = *value_size;
		memcpy(value, node + iter.value, size);
	}

	if (value_size) {
		*value_size = iter.value_size;
	}

	return CLOD_BTREE_OK;
}

enum clod_btree_result clod_btree_get(
	const uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size,
	uint8_t *value, uint8_t *value_size
) {
	if (btree_end - btree < PAGE_SIZE) return CLOD_BTREE_NO_SPACE;
	if (((uintptr_t)btree & 3) > 0) return CLOD_BTREE_NOT_ALIGNED;
	enum clod_btree_result res;

	do res = btree_get(btree, btree_end, btree, key, key_size, value, value_size);
	while (res == CLOD_BTREE_BUSY);

	return res;
}

enum clod_btree_result clod_btree_set(
	uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size,
	const uint8_t *value, uint8_t value_size
) {
	if (btree_end - btree < PAGE_SIZE) return CLOD_BTREE_NO_SPACE;
	if (((uintptr_t)btree & 3) > 0) return CLOD_BTREE_NOT_ALIGNED;
}

enum clod_btree_result clod_btree_del(
	uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size
) {
	if (btree_end - btree < PAGE_SIZE) return CLOD_BTREE_NO_SPACE;
	if (((uintptr_t)btree & 3) > 0) return CLOD_BTREE_NOT_ALIGNED;
}

enum clod_btree_result clod_btree_size(
	const uint8_t *btree, const uint8_t *btree_end,
	size_t *btree_size_out
) {
	if (btree_end - btree < PAGE_SIZE) return CLOD_BTREE_NO_SPACE;
	if (((uintptr_t)btree & 3) > 0) return CLOD_BTREE_NOT_ALIGNED;
}

enum clod_btree_result clod_btree_next(
	uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size,
	const uint8_t **next_key, uint8_t *next_key_size,
	const uint8_t **next_value, uint8_t *next_value_size
) {
	if (btree_end - btree < PAGE_SIZE) return CLOD_BTREE_NO_SPACE;
	if (((uintptr_t)btree & 3) > 0) return CLOD_BTREE_NOT_ALIGNED;
}
