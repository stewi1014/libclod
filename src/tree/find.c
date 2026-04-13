#include "config.h"
#include "debug.h"
#include <clod/tree.h>
#include "node.h"

static bool intl_search(const struct clod_tree *tree, const node *n, const char *key, int *index) {
	int low = 0;
	int high = length_get(n);

	while (low < high) {

		const int mid = (low + high) / 2;

		const int cmp = tree->compare(intl_key(n, mid), key, tree->compare_user);

		if (cmp < 0) {

			low = mid + 1;

		} else if (cmp > 0) {

			high = mid;

		} else {

			*index = mid + 1;
			return true;

		}
	}

	*index = low;
	return false;
}

static bool leaf_search(const struct clod_tree *tree, const node *n, const char *key, int *index) {
	int low = 0;
	int high = length_get(n);

	while (low < high) {

		const int mid = (low + high) / 2;
		const int cmp = tree->compare(leaf_key(n, mid), key, tree->compare_user);

		if (cmp < 0) {

			low = mid + 1;

		} else if (cmp > 0) {

			high = mid;

		} else {

			*index = mid;
			return true;

		}
	}

	*index = low;
	return false;
}

enum clod_tree_result clod_tree_find(const struct clod_tree *tree, struct clod_tree_location *location, const char *key) {
	location->node = tree->root_node;
	if (!before_read(tree, &location->node)) {
		return CLOD_TREE_CORRUPTED;
	}

	while (depth_get(location->node) > 0) {
		intl_search(tree, location->node, key, &location->index);
		location->node = intl_branch_get(location->node, location->index);
		if (!before_read(tree, &location->node)) {
			return CLOD_TREE_CORRUPTED;
		}
	}

	if (!leaf_search(tree, location->node, key, &location->index)) {
		return CLOD_TREE_NOT_FOUND;
	}

	location->key = leaf_key(location->node, location->index);
	location->val = leaf_val(location->node, location->index);
	return CLOD_TREE_OK;
}

enum clod_tree_result clod_tree_get(const struct clod_tree *tree, const char *key, char *key_out, char *val_out) {
	struct clod_tree_location location = {0};
	const enum clod_tree_result result = clod_tree_find(tree, &location, key);
	if (result == CLOD_TREE_OK) {
		copy(key_out, location.key, tree->key_size);
		copy(val_out, location.val, tree->val_size);
		return CLOD_TREE_OK;
	}
	return CLOD_TREE_NOT_FOUND;
}
