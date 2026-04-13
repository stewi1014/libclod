#include "config.h"
#include "debug.h"
#include <clod/tree.h>
#include "node.h"

enum clod_tree_result clod_tree_next(const struct clod_tree *tree, struct clod_tree_location *location) {
	if (location->node == nullptr) {
		location->node = tree->root_node;

		if (!before_read(tree, &location->node)) {
			return CLOD_TREE_CORRUPTED;
		}

		for (int i = depth_get(location->node); i > 0; i--) {
			location->node = intl_branch_get(location->node, 0);
			if (!before_read(tree, &location->node)) {
				return CLOD_TREE_CORRUPTED;
			}
		}

		location->index = 0;
	} else {
		location->index++;
	}

read:
	if (location->index < length_get(location->node)) {
		location->key = leaf_key(location->node, location->index);
		location->val = leaf_val(location->node, location->index);
		return CLOD_TREE_OK;
	}

	location->node = leaf_next_get(location->node);
	if (location->node == nullptr) {
		location->index = 0;
		location->key = nullptr;
		location->val = nullptr;
		return CLOD_TREE_NOT_FOUND;
	}

	if (!before_read(tree, &location->node)) {
		return CLOD_TREE_CORRUPTED;
	}

	location->index = 0;
	goto read;
}

enum clod_tree_result clod_tree_prev(const struct clod_tree *tree, struct clod_tree_location *location) {
	if (location->node == nullptr) {
		location->node = tree->root_node;

		if (!before_read(tree, &location->node)) {
			return CLOD_TREE_CORRUPTED;
		}

		for (int i = depth_get(location->node); i > 0; i--) {
			location->node = intl_branch_get(location->node, length_get(location->node));
			if (!before_read(tree, &location->node)) {
				return CLOD_TREE_CORRUPTED;
			}
		}

		location->index = length_get(location->node) - 1;
	} else {
		location->index--;
	}

read:
	if (location->index >= 0) {
		location->key = leaf_key(location->node, location->index);
		location->val = leaf_val(location->node, location->index);
		return CLOD_TREE_OK;
	}

	location->node = leaf_prev_get(location->node);
	if (location->node == nullptr) {
		location->index = 0;
		location->key = nullptr;
		location->val = nullptr;
		return CLOD_TREE_NOT_FOUND;
	}

	if (!before_read(tree, &location->node)) {
		return CLOD_TREE_CORRUPTED;
	}

	location->index = length_get(location->node) - 1;
	goto read;
}
