#include "config.h"
#include "debug.h"
#include <clod/tree.h>
#include "node.h"

bool clod_tree_set_node_size(struct clod_tree *tree, const unsigned short node_size) {
	if (node_size < intl_size(tree->key_size, 1)) {
		debug(CLOD_DEBUG, "Internal node size of %u is too small. Minimum internal node size is %u with a key size of %u.",
			node_size, intl_size(tree->key_size, 1), tree->key_size);
	}
	tree->intl_cap = 0;
	for (unsigned short i = 1 << 15; i > 0; i >>= 1) {
		const size_t size = intl_size(tree->key_size, tree->intl_cap | i);
		if (size <= node_size) {
			tree->intl_cap |= i;
		}
	}

	if (node_size < leaf_size(tree->key_size, tree->val_size, 1)) {
		debug(CLOD_DEBUG, "Leaf node size of %u is too small. Minimum leaf node size is %u with a key size of %u and a value size of %u.",
			node_size, leaf_size(tree->key_size, tree->val_size, 1), tree->key_size, tree->val_size);
	}
	tree->leaf_cap = 0;
	for (unsigned short i = 1 << 15; i > 0; i >>= 1) {
		const size_t size = leaf_size(tree->key_size, tree->val_size, tree->leaf_cap | i);
		if (size <= node_size) {
			tree->leaf_cap |= i;
		}
	}

	return
		intl_size(tree->key_size, tree->intl_cap) <= node_size &&
		leaf_size(tree->key_size, tree->val_size, tree->leaf_cap) <= node_size;
}

enum clod_tree_result clod_tree_create(struct clod_tree *tree) {
	if (tree->key_size == 0) {
		debug(CLOD_DEBUG, "Tree key size cannot be zero.");
		return CLOD_TREE_INVALID;
	}

	if (tree->intl_cap < 1) {
		debug(CLOD_DEBUG, "Tree internal node capacity cannot be zero.");
		return CLOD_TREE_INVALID;
	}

	if (tree->leaf_cap < 1) {
		debug(CLOD_DEBUG, "Tree leaf node capacity cannot be zero.");
		return CLOD_TREE_INVALID;
	}

	if (tree->shadow) {
		debug(CLOD_DEBUG, "Tree shadow paging isn't supported yet.");
		return CLOD_TREE_INVALID;
	}

	if (!tree->compare) {
		debug(CLOD_DEBUG, "Tree compare function cannot be null.");
		return CLOD_TREE_INVALID;
	}

	if (!tree->root_node) {
		if (!tree->allocator.allocate || !tree->allocator.free) {
			debug(CLOD_DEBUG, "Tree allocator methods cannot be null.");
			return CLOD_TREE_INVALID;
		}

		tree->root_node = tree->allocator.allocate(INTL_SIZE > LEAF_SIZE ? INTL_SIZE : LEAF_SIZE, tree->allocator.self);
		if (!tree->root_node) {
			debug(CLOD_DEBUG, "Failed to allocate root node.");
			return CLOD_TREE_ALLOC_FAILED;
		}

		depth_set(tree->root_node, 0);
		length_set(tree->root_node, 0);
		parent_set(tree->root_node, nullptr);
	}

	return CLOD_TREE_OK;
}

void clod_tree_destroy(struct clod_tree *tree) {
	if (!tree->root_node) {
		return;
	}

	node *n = tree->root_node;
	while (n) {
		const int len = length_get(n);

		if (
			depth_get(n) == 0 ||
			(length_get(n) == 0 && intl_branch_get(n, 0) == nullptr)
		) {
			node *tmp = parent_get(n);
			tree->allocator.free(n, tree->allocator.self);
			n = tmp;
			continue;
		}

		node *next = intl_branch_get(n, len);
		intl_branch_set(n, len, nullptr);
		if (len > 0) {
			length_set(n, len - 1);
		}
		n = next;
	}

	tree->root_node = nullptr;
}
