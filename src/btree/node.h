#ifndef LIBCLOD_NODE_H
#define LIBCLOD_NODE_H

#include "config.h"
#include "debug.h"
#include <clod/btree.h>

#define KEY_SIZE (tree->key_size)
#define VAL_SIZE (tree->val_size)

#define INTL_SIZE (tree->node_size)
#define LEAF_SIZE (tree->node_size)

#define INTL_CAPACITY ((unsigned short)((INTL_SIZE - 24 - 8) / (8 + KEY_SIZE)))
#define LEAF_CAPACITY ((unsigned short)((LEAF_SIZE - 40) / (KEY_SIZE + VAL_SIZE)))

/**
 *
 * # Pointers
 * Pointers are stores as an offset from the root node.
 * This means that the root node can never be moved in memory without also moving the entire tree.
 * The main reason for this is it allows the tree to be stored in a file.
 *
 * # Internal Nodex
 *
 * | Off | Size                  | Description                         |
 * |-----|-----------------------|-------------------------------------|
 * | 0   | 8                     | Reserved (future shadow paging)     |
 * | 8   | 4                     | Checksum/Canary value               |
 * | 12  | 1                     | Depth (0 < Depth < root_node_depth) |
 * | 14  | 2                     | Length of the node (key count)      |
 * | 16  | 8                     | Pointer to parent node              |
 * | 24  | key_size * max_length | Keys                                |
 * | ... | 8 * (max_length + 1)  | Pointers to child nodes             |
 *
 * # Leaf Nodes
 *
 * | Off | Size                  | Description                     |
 * |-----|-----------------------|---------------------------------|
 * | 0   | 8                     | Reserved (future shadow paging) |
 * | 8   | 4                     | Checksum/Canary value           |
 * | 12  | 1                     | Depth (always 0 for leaf nodes) |
 * | 14  | 2                     | Length of the node (key count)  |
 * | 16  | 8                     | Pointer to parent node          |
 * | 24  | 8                     | Pointer to next leaf node       |
 * | 32  | 8                     | Pointer to previous leaf node   |
 * | 40  | key_size * max_length | Keys                            |
 * | ... | val_size * max_length | Values                          |
 *
 */
typedef unsigned char node;

#define checksum_get(n) ((uint32_t)(\
	(uint32_t)(n)[8] << 24 |\
	(uint32_t)(n)[9] << 16 |\
	(uint32_t)(n)[10] << 8 |\
	(uint32_t)(n)[11]\
))

#define checksum_set(n, val) (\
	(n)[8] = (unsigned char)((val) >> 24),\
	(n)[9] = (unsigned char)((val) >> 16),\
	(n)[10] = (unsigned char)((val) >> 8),\
	(n)[11] = (unsigned char)(val)\
)

#define depth_get(n) ((n)[12])
#define depth_set(n, val) ((n)[12] = (val))
#define length_get(n) ((n)[14] << 8 | (n)[15])
#define length_set(n, val) ((n)[14] = (unsigned char)((val) >> 8), (n)[15] = (unsigned char)(val))
#define parent_get(n) ptroff_get(tree->root_node, tree->size, (n) + 16)
#define parent_set(n, val) ptroff_set(tree->root_node, tree->size, (n) + 16, val)

#define is_leaf(n) (depth_get(n) == 0)
#define is_intl(n) (depth_get(n) > 0)

#define intl_key(n, index) ((char*)(n) + 24 + (size_t)(index) * KEY_SIZE)
#define intl_branch(n, index) ((n) + 24 + (size_t)KEY_SIZE * INTL_CAPACITY + 8 * (size_t)(index))
#define intl_branch_get(n, index) ptroff_get(tree->root_node, tree->size, intl_branch(n, index))
#define intl_branch_set(n, index, val) ptroff_set(tree->root_node, tree->size, intl_branch(n, index), val)

#define leaf_next_get(n) ptroff_get(tree->root_node, tree->size, (n) + 24)
#define leaf_next_set(n, val) ptroff_set(tree->root_node, tree->size, (n) + 24, val)
#define leaf_prev_get(n) ptroff_get(tree->root_node, tree->size, (n) + 32)
#define leaf_prev_set(n, val) ptroff_set(tree->root_node, tree->size, (n) + 32, val)
#define leaf_key(n, index) ((char*)(n) + 40 + KEY_SIZE * (size_t)(index))
#define leaf_val(n, index) ((char*)(n) + 40 + KEY_SIZE * (size_t)LEAF_CAPACITY + VAL_SIZE * (size_t)(index))

static inline void ptroff_set(const void *base, const size_t base_size, unsigned char *data, const node *ptr) {
	if (base_size > 0) {
		assert_fatal(CLOD_DEBUG, (char*)ptr >= (char*)base,
			"Pointer %ptr too small; not within valid range [%ptr, %ptr).",
			ptr, base, (void*)((char*)base + base_size));

		assert_fatal(CLOD_DEBUG, (char*)ptr < (char*)base + base_size,
			"Pointer %ptr too large; not within valid range [%ptr, %ptr).",
			ptr, base, (void*)((char*)base + base_size));
	}

	uint64_t offset = 0;
	if (ptr) {
		offset = (uint64_t)((char*)ptr - (char*)base) + 1;
	}

	data[0] = (unsigned char)(offset >> 56); data[1] = (unsigned char)(offset >> 48);
	data[2] = (unsigned char)(offset >> 40); data[3] = (unsigned char)(offset >> 32);
	data[4] = (unsigned char)(offset >> 24); data[5] = (unsigned char)(offset >> 16);
	data[6] = (unsigned char)(offset >> 8 ); data[7] = (unsigned char)offset;
}
static inline node *ptroff_get(const void *base, const size_t base_size, const unsigned char *data) {
	const uint64_t offset =
		(uint64_t)data[0] << 56 | (uint64_t)data[1] << 48 |
		(uint64_t)data[2] << 40 | (uint64_t)data[3] << 32 |
		(uint64_t)data[4] << 24 | (uint64_t)data[5] << 16 |
		(uint64_t)data[6] << 8  | (uint64_t)data[7];

	if (offset == 0) {
		return nullptr;
	}

	if (base_size && offset > (uint64_t)base_size) {
		debug(CLOD_DEBUG, "Pointer offset %u64 is not within valid range %ptr to %ptr.",
			offset, base, (char*)base + base_size - 1);
		return nullptr;
	}

	return (node*)((char*)base + offset - 1);
}
static inline void copy(void *dst, const void *src, const size_t size) {
	if ((char*)dst == (char*)src) {
		return;
	}

	if ((char*)dst > (char*)src && (char*)dst < (char*)src + size) {
		for (size_t i = size; i > 0; i--) {
			((char*)dst)[i - 1] = ((char*)src)[i - 1];
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			((char*)dst)[i] = ((char*)src)[i];
		}
	}
}

#endif