#include "node.h"

static enum clod_tree_result replace(const struct clod_tree *tree, const node *old, const node *new) {
	node *parent = parent_get(old);

	if (parent) {
		enum clod_tree_result res = before_modify(tree, &parent);
		if (res) {
			return res;
		}

		const int index = intl_branch_find(tree, parent, old);
		if (index >= 0) {
			intl_branch_set(parent, index, new);
		}
	}


	node *prev = is_leaf(old) ? leaf_prev_get(old) : nullptr;
	node *next = is_leaf(old) ? leaf_next_get(old) : nullptr;


	if (parent) {
		if (!before_modify(tree, &parent)) {

		}
		const int index = intl_branch_find(tree, parent, old);
		if (index >= 0) {
			intl_branch_set(parent, index, new);
		}
	}

	if (is_leaf(old)) {
		leaf_next_set(leaf_prev_get(old), new);
		leaf_prev_set(leaf_next_get(old), new);
	}

	if (is_intl(old)) {
		for (int i = 0; i < length_get(old); i++) {
			parent_set(intl_branch_get(old, i), new);
		}
	}
}

enum clod_tree_result root_insert(
	const struct clod_tree *tree,
	node *n,
	int index,
	const char *key,
	const node *left_branch,
	const node *right_branch
) {

}

enum clod_tree_result intl_insert(
	const struct clod_tree *tree,
	node *n,
	int index,
	const char *key,
	const node *left_branch,
	const node *right_branch
) {
	int len = length_get(n);
	assert_fatal(CLOD_DEBUG, index > 0 && index <= len, "Invalid index %i for node of length %u.", index, len);

	if (len < tree->intl_cap) {
		if (!before_modify(tree, &n)) {
			return CLOD_TREE_CORRUPTED;
		}

	do_insert:
		copy(intl_key(n, index + 1), intl_key(n, index), tree->key_size * (size_t)(len - index));
		copy(intl_key(n, index), key, tree->key_size);
		copy(intl_branch(n, index + 2), intl_branch(n, index + 1), 8 * (size_t)(len - index));
		intl_branch_set(n, index, left_branch);
		intl_branch_set(n, index + 1, right_branch);

		length_set(n, len + 1);
		after_modify(tree, n);
		return CLOD_TREE_OK;
	}

	int split = len / 2;
	if (index <= split) {
		split = (len + 1) / 2;
	}

	node *right = tree->allocator.allocate(INTL_SIZE, tree->allocator.self);
	if (!right) {
		return CLOD_TREE_ALLOC_FAILED;
	}

	node *parent = parent_get(n);
	if (!before_read(tree, &parent)) {
		tree->allocator.free(right, tree->allocator.self);
		return CLOD_TREE_CORRUPTED;
	}

	if (!before_modify(tree, &n)) {
		tree->allocator.free(right, tree->allocator.self);
		return CLOD_TREE_CORRUPTED;
	}

	const enum clod_tree_result res = intl_insert(
		tree,
		parent,
		intl_branch_find(tree, parent, n),
		intl_key(n, split),
		n,
		right
	);

	if (res != CLOD_TREE_OK) {
		tree->allocator.free(right, tree->allocator.self);
		return res;
	}

	draft_set(right, right);
	checksum_set(right, 0);
	depth_set(right, depth_get(n));
	length_set(right, len - split);
	parent_set(right, parent);

	length_set(n, split);

	copy(intl_key(right, 0), intl_key(n, split + 1), tree->key_size * (size_t)(len - split - 1));
	copy(intl_branch(right, 0), intl_branch(n, split), 8 * (size_t)(len - split));

	if (index >= split) {
		after_modify(tree, n);
		n = right;
		index -= split;
		len -= split;
	} else {
		after_modify(tree, right);
		len = split;
	}

	goto do_insert;
}

enum clod_tree_result leaf_insert(
	const struct clod_tree *tree,
	node *n,
	int index,
	const char *key,
	const char *val
) {
	int len = length_get(n);
	assert_fatal(CLOD_DEBUG, index >= 0 && index <= len, "Invalid index %i for node of length %u.", index, len);

	if (len < tree->leaf_cap) {
		if (!before_modify(tree, &n)) {
			return CLOD_TREE_CORRUPTED;
		}

	do_insert:
		copy(leaf_key(n, index + 1), leaf_key(n, index), tree->key_size * (size_t)(len - index));
		copy(leaf_key(n, index), key, tree->key_size);
		copy(leaf_val(n, index + 1), leaf_val(n, index), tree->val_size * (size_t)(len - index));
		copy(leaf_val(n, index), val, tree->val_size);

		length_set(n, len + 1);
		after_modify(tree, n);
		return CLOD_TREE_OK;
	}

	int split = len / 2;
	if (index <= split) {
		split = (len + 1) / 2;
	}

	node *right = tree->allocator.allocate(LEAF_SIZE, tree->allocator.self);
	if (!right) {
		return CLOD_TREE_ALLOC_FAILED;
	}

	node *parent = parent_get(n);
	int parent_index = 0;
	if (parent) {
		if (!before_read(tree, &parent)) {
			tree->allocator.free(right, tree->allocator.self);
			return CLOD_TREE_CORRUPTED;
		}

		parent_index = intl_branch_find(tree, parent, n);
	}

	if (!before_modify(tree, &n)) {
		tree->allocator.free(right, tree->allocator.self);
		return CLOD_TREE_CORRUPTED;
	}

	const enum clod_tree_result res = intl_insert(tree, parent, parent_index, leaf_key(n, split), n, right);
	if (res != CLOD_TREE_OK) {
		tree->allocator.free(right, tree->allocator.self);
		return res;
	}

	draft_set(right, right);
	checksum_set(right, 0);
	depth_set(right, 0);
	length_set(right, len - split);
	parent_set(right, parent);

	length_set(n, split);

	copy(leaf_key(right, 0), leaf_key(n, split), tree->key_size * (size_t)(len - split));
	copy(leaf_val(right, 0), leaf_val(n, split), tree->val_size * (size_t)(len - split));

	if (index >= split) {
		after_modify(tree, n);
		n = right;
		index -= split;
		len -= split;
	} else {
		after_modify(tree, right);
		len = split;
	}

	goto do_insert;
}

/*
enum clod_tree_result clod_tree_insert(const struct clod_tree *tree, struct clod_tree_location *location) {
	if (location->node == nullptr) {
		clod_tree_find(tree, location, location->key);
	}
}
*/

enum clod_tree_result clod_tree_add(const struct clod_tree *tree, const char *key, const char *val, char *key_out, char *val_out) {
	struct clod_tree_location location = {0};
	enum clod_tree_result result = clod_tree_find(tree, &location, key);
	if (result == CLOD_TREE_OK) {
		copy(key_out, location.key, tree->key_size);
		copy(val_out, location.val, tree->val_size);
		return CLOD_TREE_ALREADY_EXISTS;
	}

	return leaf_insert(tree, location.node, location.index, key, val);
}

/*

/// Get the ancestor and key index which delimits the left of the given node.
static node *left_fork(const struct clod_tree *tree, node *n, int *key_index) {
	int index;
	do {
		node *parent = parent_get(n);
		if (parent == nullptr) {
			return nullptr;
		}

		index = 0;
		while (index <= length_get(parent) && intl_branch_get(parent, index) != n) {
			index++;
		}

		if (index > length_get(parent)) {
			return nullptr;
		}

		n = parent;
	} while (index == 0);

	*key_index = index - 1;
	return n;
}

/// Get the ancestor and key index which delimits the right of the given node.
static node *right_fork(const struct clod_tree *tree, node *n, int *key_index) {
	int index;
	do {
		node *parent = parent_get(n);
		if (parent == nullptr) {
			return nullptr;
		}

		index = length_get(parent);
		while (index >= 0 && intl_branch_get(parent, index) != n) {
			index--;
		}

		if (index < 0) {
			return nullptr;
		}

		n = parent;
	} while (index == length_get(n));

	*key_index = index;
	return n;
}

enum clod_tree_result leaf_insert(
	const struct clod_tree *tree,
	node *const n,
	const int index,
	const char *key,
	const char *val
) {
	const int len = length_get(n);

	assert_fatal(CLOD_DEBUG, index < tree->leaf_cap,
		"Index %u out of bounds to insert into leaf node %ptr with length %u (capacity %u).",
		index, (void*)n, tree->leaf_cap);

	// Don't allow appending to nodes that aren't the last one, because it's not a valid location.
	// The valid representation of that location is index 0 of the next node.
	assert_fatal(CLOD_DEBUG, index < len || (index == len && leaf_next_get(n) == nullptr),
		"Index %u out of bounds to insert into leaf node %ptr with length %u (capacity %u).",
		index, (void*)n, len, tree->leaf_cap);

	if (len < tree->leaf_cap) {
		if (!before_modify(tree, n, nullptr)) {
			return CLOD_TREE_CORRUPTED;
		}

		if (index < len) {
			copy(leaf_key(n, index + 1), leaf_key(n, index), tree->key_size * (size_t)(len - index));
			copy(leaf_val(n, index + 1), leaf_val(n, index), tree->val_size * (size_t)(len - index));
		}

		copy(leaf_key(n, index), key, tree->key_size);
		copy(leaf_val(n, index), val, tree->val_size);

		length_set(n, len + 1);

		after_modify(tree, n, nullptr);
		return CLOD_TREE_OK;
	}

	node *left = leaf_prev_get(n);
	if (left && length_get(left) < tree->leaf_cap) {
		const int left_len = length_get(left);

		int fork_index;
		node *fork_ancestor = left_fork(tree, n, &fork_index);

		if (index == 0) {

			if (!before_modify(tree, left, fork_ancestor, nullptr)) {
				return CLOD_TREE_CORRUPTED;
			}

			copy(leaf_key(left, left_len), key, tree->key_size);
			copy(intl_key(fork_ancestor, fork_index), key, tree->key_size);

			copy(leaf_val(left, left_len), val, tree->val_size);

			length_set(left, left_len + 1);

			after_modify(tree, left, fork_ancestor, nullptr);

		} else {

			if (!before_modify(tree, n, left, fork_ancestor, nullptr)) {
				return CLOD_TREE_CORRUPTED;
			}

			copy(leaf_key(left, left_len), leaf_key(n, 0), tree->key_size);
			copy(intl_key(fork_ancestor, fork_index), leaf_key(n, 0), tree->key_size);
			copy(leaf_key(n, 0), leaf_key(n, 1), tree->key_size * (size_t)(index - 1));
			copy(leaf_key(n, index - 1), key, tree->key_size);

			copy(leaf_val(left, left_len), leaf_val(n, 0), tree->val_size);
			copy(leaf_val(n, 0), leaf_val(n, 1), tree->val_size * (size_t)(index - 1));
			copy(leaf_val(n, index - 1), val, tree->val_size);

			length_set(left, left_len + 1);

			after_modify(tree, n, left, fork_ancestor, nullptr);
		}

		return CLOD_TREE_OK;
	}

	node *right = leaf_next_get(n);
	if (right && length_get(right) < tree->leaf_cap) {
		const int right_len = length_get(right);

		if (index == len) {

			if (!before_modify(tree, right, nullptr)) {
				return CLOD_TREE_CORRUPTED;
			}

			copy(leaf_key(right, 1), leaf_key(right, 0), tree->key_size * (size_t)right_len);
			copy(leaf_key(right, 0), key, tree->key_size);
			copy(leaf_val(right, 1), leaf_val(right, 0), tree->val_size * (size_t)right_len);
			copy(leaf_val(right, 0), val, tree->val_size);

			length_set(right, right_len + 1);

			after_modify(tree, right, nullptr);

		} else {
			int fork_index;
			node *fork_ancestor = right_fork(tree, n, &fork_index);

			if (!before_modify(tree, n, right, fork_ancestor, nullptr)) {
				return CLOD_TREE_CORRUPTED;
			}

			copy(leaf_key(right, 1), leaf_key(right, 0), tree->key_size * (size_t)right_len);
			copy(leaf_key(right, 0), leaf_key(n, len - 1), tree->key_size);
			copy(leaf_key(n, index + 1), leaf_key(n, index), tree->key_size * (size_t)(len - index - 1));
			copy(leaf_key(n, index), key, tree->key_size);
			copy(intl_key(fork_ancestor, fork_index), leaf_key(n, len - 1), tree->key_size);

			copy(leaf_val(right, 1), leaf_val(right, 0), tree->val_size * (size_t)right_len);
			copy(leaf_val(right, 0), leaf_val(n, len - 1), tree->val_size);
			copy(leaf_val(n, index + 1), leaf_val(n, index), tree->val_size * (size_t)(len - index - 1));
			copy(leaf_val(n, index), val, tree->val_size);

			length_set(right, right_len + 1);

			after_modify(tree, n, right, fork_ancestor, nullptr);
		}

		return CLOD_TREE_OK;
	}

	return CLOD_TREE_ALLOC_FAILED;
}

enum clod_tree_result intl_insert(
	const struct clod_tree *tree,
	node *const n,
	const int index,
	const char *key,
	const node *branch
) {
	const int len = length_get(n);

	assert_fatal(CLOD_DEBUG, index <= len && index < tree->leaf_cap,
		"Index %u out of bounds to insert into intl node %ptr with length %u (capacity %u).",
		index, (void*)n, tree->leaf_cap);

	if (len < tree->intl_cap) {
		if (!before_modify(tree, n, nullptr)) {
			return CLOD_TREE_CORRUPTED;
		}

		if (index <= len) {
			copy(intl_key(n, index + 1), intl_key(n, index), tree->key_size * (size_t)(len - index));
			copy(intl_branch(n, index + 2), intl_branch(n, index + 1), 8 * (size_t)(len - index));
		}

		copy(intl_key(n, index), key, tree->key_size);
		intl_branch_set(n, index + 1, branch);

		length_set(n, len + 1);

		after_modify(tree, n, nullptr);
		return CLOD_TREE_OK;
	}

	node *parent = parent_get(n);
	const int parent_index = intl_branch_index(tree, parent, n);

	if (parent_index > 0) {

		node *left = intl_branch_get(parent, parent_index - 1);
		if (length_get(left) < tree->intl_cap) {

			if (!before_modify(tree, left, parent, nullptr)) {
				return CLOD_TREE_CORRUPTED;
			}



		}

	}
}

*/
