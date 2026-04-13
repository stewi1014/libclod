#include "config.h"
#include "debug.h"
#include <clod/tree.h>
#include "node.h"

#define TYPE_NONE 0
#define TYPE_LEAF 1
#define TYPE_INTL 2

#define KEY_SIZE (tree->key_size)
#define VAL_SIZE (tree->val_size)
#define INTL_CAPACITY (tree->intl_cap)
#define LEAF_CAPACITY (tree->leaf_cap)

/// Header for each node.
/// All values are big-endian encoded.
typedef struct clod_tree_node {
	unsigned char header[32];
	char keys[];
} node_old;

#define CAPACITY(n) (type_get_old(n) == TYPE_LEAF ? LEAF_CAPACITY : type_get_old(n) == TYPE_INTL ? INTL_CAPACITY : 0)

#define type_get_old(n) ((n)->header[4])
#define type_set_old(n, val) ((n)->header[4] = (val), (n)->header[5] = (val) == 1 ? 0 : 1)
#define length_get_old(n) length_get((node*)n)
#define length_set_old(n, val) length_set((node*)n, val)
#define parent_get_old(n) (node_old*)parent_get((node*)n)
#define parent_set_old(n, val) parent_set((node*)n, val)
#define next_get_old(n) (node_old*)leaf_next_get((node*)n)
#define next_set_old(n, val) leaf_next_set((node*)n, val)
#define prev_get_old(n) (node_old*)leaf_prev_get((node*)n)
#define prev_set_old(n, val) leaf_prev_set((node*)n, val)
#define key_get_old(n, index) (depth_get((node*)n) > 0 ? intl_key((node*)n, index) : leaf_key((node*)n, index))
#define key_set_old(n, index, val) copy(key_get_old((node*)n, index), val, tree->key_size);
#define val_get(n, index) leaf_val((node*)n, index)
#define val_set(n, index, val) copy(val_get((node*)n, index), val, tree->val_size)
#define branch_get_old(n, index) (node_old*)intl_branch_get((node*)n, index)
#define branch_set_old(n, index, val) intl_branch_set((node*)n, index, val)

/// Return the index of the key in \p n which compares equal to, or the closest value higher than, key.
/// @return true if an exact match was found.
static bool search(const struct clod_tree *tree, const node_old *n, const void *key, int *index) {
	int l = 0, h = length_get_old(n);
	while (l < h) {
		const int m = (l + h) / 2;
		const int cmp = tree->compare(key_get_old(n, m), key, tree->compare_user);
		if (cmp < 0) {
			l = m + 1;
		} else if (cmp > 0) {
			h = m;
		} else {
			*index = m;
			return true;
		}
	}

	*index = l;
	return false;
}

/// Get the parent and index of a given node.
static node_old *parent_branch_get_old(const struct clod_tree *tree, const node_old *n, int *parent_index) {
	node_old *parent = parent_get_old(n);
	if (!parent) {
		return nullptr;
	}

	for (int i = 0; i <= length_get_old(parent); i++) {
		if (branch_get_old(parent, i) == n) {
			if (parent_index) *parent_index = i;
			return parent;
		}
	}

	assert_fatal(CLOD_DEBUG, false, "Parent of %ptr does not contain it! Invalid tree.", (void*)n);
}

/// Insert a key and value into the given leaf node,
static void leaf_insert_old(const struct clod_tree *tree, node_old *n, int index, const void *key, const void *val) {
	const int len = length_get_old(n);
	assert_fatal(CLOD_DEBUG, index <= len,
		"Index %i out of bounds for inserting into node %ptr with %i keys.",
		index, (void*)n, len);

	for (int i = len; i > index; i--) key_set_old(n, i, key_get_old(n, i - 1));
	for (int i = len; i > index; i--) val_set(n, i, val_get(n, i - 1));

	length_set_old(n, len + 1);
	key_set_old(n, index, key);
	val_set(n, index, val);

	if (index != len) {
		return;
	}

	node_old *parent = parent_branch_get_old(tree, n, &index);
	while (parent && index == length_get_old(parent)) {
		parent = parent_branch_get_old(tree, parent, &index);
	}

	if (parent) {
		key_set_old(parent, index, key_get_old(n, len));
	}
}

/// Insert a key and branch into the given internal node, with the branch being placed to the left of the new key.
static void intl_insert_left(const struct clod_tree *tree, node_old *n, const int index, const void *key, node_old *branch) {
	for (int i = length_get_old(n) + 1; i > index; i--) branch_set_old(n, i, branch_get_old(n, i - 1));
	for (int i = length_get_old(n); i > index; i--) key_set_old(n, i, key_get_old(n, i - 1));

	length_set_old(n, length_get_old(n) + 1);
	key_set_old(n, index, key);
	branch_set_old(n, index, branch);
	parent_set_old(branch, n);
}

/// Insert a key and branch into the given internal node, with the branch being placed to the right of the new key.
static void intl_insert_right(const struct clod_tree *tree, node_old *n, const int index, const void *key, node_old *branch) {
	for (int i = length_get_old(n) + 1; i > index + 1; i--) branch_set_old(n, i, branch_get_old(n, i - 1));
	for (int i = length_get_old(n); i > index; i--) key_set_old(n, i, key_get_old(n, i - 1));

	length_set_old(n, length_get_old(n) + 1);
	key_set_old(n, index, key);
	branch_set_old(n, index + 1, branch);
	parent_set_old(branch, n);
}

/// Remove the element in the given leaf node, and remove the
/// instance of its key in the tree if one exists.
static void leaf_remove(const struct clod_tree *tree, node_old *const n, int index) {
	const int len = length_get_old(n);
	for (int i = index; i < len - 1; i++) val_set(n, i, val_get(n, i + 1));
	for (int i = index; i < len - 1; i++) key_set_old(n, i, key_get_old(n, i + 1));
	length_set_old(n, len - 1);

	if (index != len - 1) {
		return;
	}

	node_old *parent = parent_branch_get_old(tree, n, &index);
	while (parent && index == length_get_old(parent)) {
		parent = parent_branch_get_old(tree, parent, &index);
	}

	if (parent) {
		key_set_old(parent, index, key_get_old(n, len - 2));
	}
}

/// Remove a key and branch from the given internal node, with the branch to the left of the key being removed.
static void intl_remove_left(const struct clod_tree *tree, node_old *const n, const int index) {
	for (int i = index; i < length_get_old(n); i++) branch_set_old(n, i, branch_get_old(n, i + 1));
	for (int i = index; i < length_get_old(n) - 1; i++) key_set_old(n, i, key_get_old(n, i + 1));
	length_set_old(n, length_get_old(n) - 1);
}

/// Remove a key and branch from the given internal node, with the branch to the right of the key being removed.
static void intl_remove_right(const struct clod_tree *tree, node_old *const n, const int index) {
	for (int i = index + 1; i < length_get_old(n); i++) branch_set_old(n, i, branch_get_old(n, i + 1));
	for (int i = index; i < length_get_old(n) - 1; i++) key_set_old(n, i, key_get_old(n, i + 1));
	length_set_old(n, length_get_old(n) - 1);
}

/// Split the elements in \p n between \p left and \p right.
/// @return index in \p n of the promoted key.
static int leaf_split(const struct clod_tree *tree, const node_old *n, node_old *left, node_old *right) {
	const int keys = length_get_old(n);
	const int left_size = (keys + 1) / 2;
	const int right_size = keys - left_size;

	assert_fatal(CLOD_DEBUG, keys >= LEAF_CAPACITY / 2,
		"Refusing to split leaf node %ptr with %i keys. Nodes may never be less than half full.",
		(void*)n, keys);

	int parent_index;
	node_old *const parent = parent_branch_get_old(tree, n, &parent_index);
	assert_fatal(CLOD_DEBUG, !parent || length_get_old(parent) < INTL_CAPACITY,
		"Cannot split leaf node %ptr when its parent %ptr has %i out of %i keys occupied.",
		(void*)n, (void*)parent, length_get_old(parent), INTL_CAPACITY);

	type_set_old(left, TYPE_LEAF);
	type_set_old(right, TYPE_LEAF);
	parent_set_old(left, parent);
	parent_set_old(right, parent);
	const node_old *n_next = next_get_old(n), *n_prev = prev_get_old(n);
	next_set_old(left, right);
	next_set_old(right, n_next);
	prev_set_old(left, n_prev);
	prev_set_old(right, left);

	if (parent) {
		branch_set_old(parent, parent_index, left);
		intl_insert_right(tree, parent, parent_index, key_get_old(n, left_size - 1), right);
	}

	if (n == left) {
		for (int i = 0; i < right_size; i++) key_set_old(right, i, key_get_old(n, i + left_size));
		for (int i = 0; i < right_size; i++) val_set(right, i, val_get(n, i + left_size));
	} else {
		for (int i = 0; i < left_size; i++) key_set_old(left, i, key_get_old(n, i));
		for (int i = 0; i < right_size; i++) key_set_old(right, i, key_get_old(n, i + left_size));
		for (int i = 0; i < left_size; i++) val_set(left, i, val_get(n, i));
		for (int i = 0; i < right_size; i++) val_set(right, i, val_get(n, i + left_size));
	}

	length_set_old(left, left_size);
	length_set_old(right, right_size);
	return left_size - 1;
}

/// Split the elements in \p n between \p left and \p right.
/// @return index in \p n of the promoted key.
static int intl_split(const struct clod_tree *tree, const node_old *n, node_old *left, node_old *right) {
	const int keys = length_get_old(n);
	const int left_size = keys / 2;
	const int right_size = keys - left_size - 1;

	assert_fatal(CLOD_DEBUG, keys >= INTL_CAPACITY / 2,
		"Refusing to split internal node %ptr with %i keys. Nodes may never be less than half full.",
		(void*)n, keys);

	int parent_index;
	node_old *const parent = parent_branch_get_old(tree, n, &parent_index);
	assert_fatal(CLOD_DEBUG, !parent || length_get_old(parent) < INTL_CAPACITY,
		"Cannot split leaf node when its parent %ptr has no space.", (void*)parent);

	type_set_old(left, TYPE_INTL);
	type_set_old(right, TYPE_INTL);
	parent_set_old(left, parent);
	parent_set_old(right, parent);
	if (parent) {
		branch_set_old(parent, parent_index, left);
		intl_insert_right(tree, parent, parent_index, key_get_old(n, left_size), right);
	}

	if (n == left) {
		for (int i = 0; i < right_size; i++) key_set_old(right, i, key_get_old(n, i + left_size + 1));
		for (int i = 0; i <= right_size; i++) branch_set_old(right, i, branch_get_old(n, i + left_size + 1));
	} else {
		for (int i = 0; i < left_size; i++) key_set_old(left, i, key_get_old(n, i));
		for (int i = 0; i < right_size; i++) key_set_old(right, i, key_get_old(n, i + left_size + 1));
		for (int i = 0; i <= left_size; i++) branch_set_old(left, i, branch_get_old(n, i));
		for (int i = 0; i <= right_size; i++) branch_set_old(right, i, branch_get_old(n, i + left_size + 1));
	}

	length_set_old(left, left_size);
	length_set_old(right, right_size);
	for (int i = 0; i <= left_size; i++) parent_set_old(branch_get_old(left, i), left);
	for (int i = 0; i <= right_size; i++) parent_set_old(branch_get_old(right, i), right);

	return left_size;
}

/// Copies the elements from \p left and \p right into \p n.
static void leaf_merge(const struct clod_tree *tree, node_old *n, const node_old *left, const node_old *right) {
	const int left_len = length_get_old(left);
	const int right_len = length_get_old(right);

	assert_fatal(CLOD_DEBUG, left_len + right_len <= LEAF_CAPACITY,
		"Cannot merge leaf nodes %ptr and %ptr because their combined elements %i + %i = %i is larger than the maximum %i.",
		(void*)left, (void*)right, left_len, right_len, left_len + right_len, LEAF_CAPACITY);

	int parent_index;
	node_old *parent = parent_branch_get_old(tree, left, &parent_index);
	assert_fatal(CLOD_DEBUG, parent == parent_get_old(right),
		"Cannot merge leaf nodes %ptr and %ptr that belong to different parents %ptr and %ptr.",
		(void*)left, (void*)right, (void*)parent_get_old(left), (void*)parent_get_old(right));

	type_set_old(n, TYPE_LEAF);
	length_set_old(n, left_len + right_len);
	parent_set_old(n, parent);
	if (prev_get_old(left)) next_set_old(prev_get_old(left), n);
	if (next_get_old(right)) prev_set_old(next_get_old(right), n);
	next_set_old(n, next_get_old(right));
	prev_set_old(n, prev_get_old(left));

	intl_remove_right(tree, parent, parent_index);
	branch_set_old(parent, parent_index, n);

	if (n == left) {
		for (int i = 0; i < right_len; i++) key_set_old(n, i + left_len, key_get_old(right, i));
		for (int i = 0; i < right_len; i++) val_set(n, i + left_len, val_get(right, i));
	} else {
		for (int i = right_len - 1; i >= 0; i--) key_set_old(n, i + left_len, key_get_old(right, i));
		for (int i = 0; i < left_len; i++) key_set_old(n, i, key_get_old(left, i));
		for (int i = right_len - 1; i >= 0; i--) val_set(n, i + left_len, val_get(right, i));
		for (int i = 0; i < left_len; i++) val_set(n, i, val_get(left, i));
	}
}

/// Copies the elements from \p left and \p right into \p n.
/// @return index in \p n of the demoted key.
static int intl_merge(const struct clod_tree *tree, node_old *n, const node_old *left, const node_old *right) {
	const int left_len = length_get_old(left);
	const int right_len = length_get_old(right);

	assert_fatal(CLOD_DEBUG, left_len + right_len + 1 <= INTL_CAPACITY,
		"Cannot merge internal nodes %ptr and %ptr because their combined elements %i + %i = %i is larger than the maximum %i.",
		(void*)left, (void*)right, left_len, right_len, left_len + right_len, INTL_CAPACITY);

	int parent_index;
	node_old *parent = parent_branch_get_old(tree, left, &parent_index);
	assert_fatal(CLOD_DEBUG, parent == parent_get_old(right),
		"Cannot merge internal nodes %ptr and %ptr that belong to different parents %ptr and %ptr.",
		(void*)left, (void*)right, (void*)parent_get_old(left), (void*)parent_get_old(right));

	type_set_old(n, TYPE_INTL);
	length_set_old(n, left_len + right_len + 1);
	parent_set_old(n, parent);

	if (n == left) {
		for (int i = 0; i < right_len; i++) key_set_old(n, i + left_len + 1, key_get_old(right, i));
		for (int i = 0; i <= right_len; i++) branch_set_old(n, i + left_len + 1, branch_get_old(right, i));
	} else {
		for (int i = right_len - 1; i >= 0; i--) key_set_old(n, i + left_len + 1, key_get_old(right, i));
		for (int i = 0; i < left_len; i++) key_set_old(n, i, key_get_old(left, i));
		for (int i = right_len - 1; i >= 0; i--) branch_set_old(n, i + left_len + 1, branch_get_old(right, i));
		for (int i = 0; i <= left_len; i++) branch_set_old(n, i, branch_get_old(left, i));
	}

	key_set_old(n, left_len, key_get_old(parent, parent_index));
	for (int i = 0; i <= length_get_old(n); i++) parent_set_old(branch_get_old(n, i), n);
	intl_remove_right(tree, parent, parent_index);
	branch_set_old(parent, parent_index, n);

	return left_len;
}

/// Balance the given node.
static void balance(const struct clod_tree *tree, node_old *n) {
	int index;
	node_old *parent = parent_branch_get_old(tree, n, &index);
	if (!parent) {
		debug(CLOD_DEBUG, "Cannot balance root node.");
		return;
	}

	node_old *left, *right;
	if (index == 0) {
		left = n;
		right = branch_get_old(parent, 1);
	} else {
		left = branch_get_old(parent, index - 1);
		right = n;
	}
	const int left_len = length_get_old(left), right_len = length_get_old(right);

	if (type_get_old(n) == TYPE_LEAF) {
		if (left_len + right_len <= LEAF_CAPACITY) {

			leaf_merge(tree, left, left, right);
			tree->allocator.free(right, tree->allocator.self);


		} else if (left_len < right_len) {

			leaf_insert_old(tree, left, left_len, key_get_old(right, 0), val_get(right, 0));
			leaf_remove(tree, right, 0);

		} else if (right_len < left_len) {

			leaf_insert_old(tree, right, 0, key_get_old(left, left_len - 1), val_get(left, left_len - 1));
			leaf_remove(tree, left, left_len - 1);

		}
	} else if (type_get_old(n) == TYPE_INTL) {

		if (left_len + right_len + 1 <= INTL_CAPACITY) {

			intl_merge(tree, left, left, right);
			tree->allocator.free(right, tree->allocator.self);

		} else if (left_len < right_len) {

			intl_insert_right(tree, left, left_len, key_get_old(parent, index), branch_get_old(right, 0));
			key_set_old(parent, index, key_get_old(right, 0));
			intl_remove_left(tree, right, 0);

		} else if (right_len < left_len) {

			intl_insert_left(tree, right, 0, key_get_old(parent, index), branch_get_old(left, left_len - 1));
			key_set_old(parent, index, key_get_old(left, left_len - 2));
			intl_remove_right(tree, left, left_len - 1);

		}

	}
}

enum clod_tree_result clod_tree_add(const struct clod_tree *tree, const char *key, const char *val, char *key_out, char *val_out) {
	node_old *n = (node_old*)tree->root_node;
	int index;

	while (type_get_old(n) == TYPE_INTL) {
		search(tree, n, key, &index);
		n = branch_get_old(n, index);
	}

	if (search(tree, n, key, &index)) {
		if (key_out) copy(key_out, key_get_old(n, index), tree->key_size);
		if (val_out) copy(val_out, val_get(n, index), tree->val_size);
		return CLOD_TREE_ALREADY_EXISTS;
	}

	if (length_get_old(n) < LEAF_CAPACITY) {
		leaf_insert_old(tree, n, index, key, val);
		return CLOD_TREE_OK;
	}

	int parent_index;
	node_old *parent = parent_branch_get_old(tree, n, &parent_index);
	if (parent && parent_index > 0 && length_get_old(branch_get_old(parent, parent_index - 1)) < LEAF_CAPACITY) {
		node_old *left = branch_get_old(parent, parent_index - 1);
		if (index == 0) {
			leaf_insert_old(tree, left, length_get_old(left), key, val);
		} else {
			leaf_insert_old(tree, left, length_get_old(left), key_get_old(n, 0), val_get(n, 0));
			leaf_remove(tree, n, 0);
			leaf_insert_old(tree, n, index - 1, key, val);
		}

		return CLOD_TREE_OK;
	}

	if (parent && parent_index < length_get_old(parent) && length_get_old(branch_get_old(parent, parent_index + 1)) < LEAF_CAPACITY) {
		node_old *right = branch_get_old(parent, parent_index + 1);

		leaf_insert_old(tree, right, 0, key_get_old(n, length_get_old(n)), val_get(n, length_get_old(n)));
		leaf_remove(tree, n, length_get_old(n));
		leaf_insert_old(tree, n, index, key, val);

		return CLOD_TREE_OK;
	}

	while (parent_get_old(n) && length_get_old(parent_get_old(n)) >= INTL_CAPACITY) {
		parent = parent_branch_get_old(tree, n, &parent_index);

		if (parent_index > 0 && length_get_old(branch_get_old(parent, parent_index - 1)) < INTL_CAPACITY) {
			node_old *left = branch_get_old(parent, parent_index - 1);
			intl_insert_right(tree, left, length_get_old(left), key_get_old(parent, parent_index - 1), branch_get_old(n, 0));
			key_set_old(parent, parent_index - 1, key_get_old(n, 0));
			intl_remove_left(tree, n, 0);

			search(tree, parent, key, &index);
			n = branch_get_old(parent, index);
			search(tree, n, key, &index);
			n = branch_get_old(n, index);
			break;
		}

		if (parent_index < length_get_old(parent) - 1 && length_get_old(branch_get_old(parent, parent_index + 1)) < INTL_CAPACITY) {
			node_old *right = branch_get_old(parent, parent_index + 1);
			intl_insert_left(tree, right, 0, key_get_old(parent, parent_index), branch_get_old(n, length_get_old(n)));
			key_set_old(parent, parent_index, key_get_old(n, length_get_old(n) - 1));
			intl_remove_right(tree, n, length_get_old(n) - 1);

			search(tree, parent, key, &index);
			n = branch_get_old(parent, index);
			search(tree, n, key, &index);
			n = branch_get_old(n, index);
			break;
		}

		n = parent_get_old(n);
	}

	if (parent_get_old(n) == nullptr) {
		if (type_get_old(n) == TYPE_LEAF) {
			node_old *left = tree->allocator.allocate(LEAF_SIZE, tree->allocator.self);
			node_old *right = tree->allocator.allocate(LEAF_SIZE, tree->allocator.self);
			if (!left || !right) {
				debug(CLOD_DEBUG, "Failed to allocate new internal node.");
				if (left) tree->allocator.free(left, tree->allocator.self);
				if (right) tree->allocator.free(right, tree->allocator.self);
				return CLOD_TREE_ALLOC_FAILED;
			}

			const int split = leaf_split(tree, n, left, right);

			key_set_old(n, 0, key_get_old(n, split));
			type_set_old(n, TYPE_INTL);
			depth_set((node*)n, 1);
			length_set_old(n, 1);
			branch_set_old(n, 0, left);
			branch_set_old(n, 1, right);
			parent_set_old(left, n);
			parent_set_old(right, n);

			search(tree, n, key, &index);
			n = branch_get_old(n, index);
			search(tree, n, key, &index);
			leaf_insert_old(tree, n, index, key, val);
			return CLOD_TREE_OK;
		}

		if (type_get_old(n) == TYPE_INTL) {
			node_old *left = tree->allocator.allocate(INTL_SIZE, tree->allocator.self);
			node_old *right = tree->allocator.allocate(INTL_SIZE, tree->allocator.self);
			if (!left || !right) {
				debug(CLOD_DEBUG, "Failed to allocate new internal node.");
				if (left) tree->allocator.free(left, tree->allocator.self);
				if (right) tree->allocator.free(right, tree->allocator.self);
				return CLOD_TREE_ALLOC_FAILED;
			}

			const int split = intl_split(tree, n, left, right);

			key_set_old(n, 0, key_get_old(n, split));
			depth_set((node*)n, depth_get((node*)n) + 1);
			length_set_old(n, 1);
			branch_set_old(n, 0, left);
			branch_set_old(n, 1, right);
			parent_set_old(left, n);
			parent_set_old(right, n);

			search(tree, n, key, &index);
			n = branch_get_old(n, index);
			search(tree, n, key, &index);
			n = branch_get_old(n, index);
		}
	}

	while (type_get_old(n) == TYPE_INTL) {
		node_old *new = tree->allocator.allocate(INTL_SIZE, tree->allocator.self);
		if (!new) {
			debug(CLOD_DEBUG, "Failed to allocate new internal node.");
			return CLOD_TREE_ALLOC_FAILED;
		}

		search(tree, n, key, &index);
		const int split = intl_split(tree, n, n, new);
		if (index < split) {
			n = branch_get_old(n, index);
		} else {
			n = branch_get_old(new, index - split - 1);
		}
	}

	node_old *new = tree->allocator.allocate(LEAF_SIZE, tree->allocator.self);
	if (!new) {
		debug(CLOD_DEBUG, "Failed to allocate new leaf node.");
		return CLOD_TREE_ALLOC_FAILED;
	}

	search(tree, n, key, &index);
	const int split = leaf_split(tree, n, n, new);
	if (index >= split) {
		n = new;
		index = index - split - 1;
	}

	leaf_insert_old(tree, n, index, key, val);
	return CLOD_TREE_OK;
}

bool clod_tree_get(const struct clod_tree *tree, const char *key, char *key_out, char *val_out) {
	const node_old *n = (node_old*)tree->root_node;
	int index;
	while (type_get_old(n) == TYPE_INTL) {
		search(tree, n, key, &index);
		n = branch_get_old(n, index);
	}

	if (search(tree, n, key, &index)) {
		if (key_out) copy(key_out, key_get_old(n, index), tree->key_size);
		if (val_out) copy(val_out, val_get(n, index), tree->val_size);
		return true;
	}
	return false;
}

bool clod_tree_del(const struct clod_tree *tree, const char *key, char *key_out, char *val_out) {
	node_old *n = (node_old*)tree->root_node;
	int index;
	while (type_get_old(n) == TYPE_INTL) {
		search(tree, n, key, &index);
		n = branch_get_old(n, index);
	}

	if (!search(tree, n, key, &index)) {
		return false;
	}

	if (key_out) copy(key_out, key_get_old(n, index), tree->key_size);
	if (val_out) copy(val_out, val_get(n, index), tree->val_size);
	leaf_remove(tree, n, index);

	while (n && parent_get_old(n) && length_get_old(n) < LEAF_CAPACITY / 2) {
		balance(tree, n);
		n = parent_get_old(n);
	}

	return true;
}
