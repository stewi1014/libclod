#include "config.h"
#include "debug.h"
#include <clod/structures/tree.h>

#define TYPE_NONE 0
#define TYPE_LEAF 1
#define TYPE_INTL 2

#define KEY_SIZE (tree->key_size)
#define VAL_SIZE (tree->val_size)
#define INTL_CAPACITY ((tree->node_size - (int)sizeof(node) - 8) / (tree->key_size + 8))
#define LEAF_CAPACITY ((tree->node_size - (int)sizeof(node)) / (tree->key_size + tree->val_size))

/// Header for each node.
/// All values are big-endian encoded.
typedef struct clod_tree_node {
	unsigned char header[32];
	char keys[];
} node;

static node *ptroff_get(const void *base, const unsigned char *data) {
	const ptrdiff_t offset = (ptrdiff_t)(
		(uint64_t)data[0] << 56 | (uint64_t)data[1] << 48 |
		(uint64_t)data[2] << 40 | (uint64_t)data[3] << 32 |
		(uint64_t)data[4] << 24 | (uint64_t)data[5] << 16 |
		(uint64_t)data[6] << 8  | (uint64_t)data[7]);
	if (offset == 0) return nullptr;
	return (node*)((char*)base + offset - 1);
}
static void ptroff_set(const void *base, unsigned char *data, const node *ptr) {
	uint64_t offset;
	if (ptr) {
		offset = (uint64_t)((char*)ptr - (char*)base) + 1;
	} else {
		offset = 0;
	}
	data[0] = (unsigned char)(offset >> 56); data[1] = (unsigned char)(offset >> 48);
	data[2] = (unsigned char)(offset >> 40); data[3] = (unsigned char)(offset >> 32);
	data[4] = (unsigned char)(offset >> 24); data[5] = (unsigned char)(offset >> 16);
	data[6] = (unsigned char)(offset >> 8 ); data[7] = (unsigned char)offset;
}
static void copy(void *dst, const void *src, const size_t n) {
	if (dst == src) return;

	assert_fatal(CLOD_DEBUG, (char*)dst + n <= (char*)src || (char*)dst >= (char*)src + n,
		"Copying key/value in tree has buffers that overlap. This implies serious implementation bug.");

	for (size_t i = 0; i < n; i++) {
		((char*)dst)[i] = ((char*)src)[i];
	}
}

#define CAPACITY(n) (type_get(n) == TYPE_LEAF ? LEAF_CAPACITY : type_get(n) == TYPE_INTL ? INTL_CAPACITY : 0)

#define checksum_get(node) (\
	(uint32_t)(node)->header[0] << 24 |\
	(uint32_t)(node)->header[1] << 16 |\
	(uint32_t)(node)->header[2] << 8  |\
	(uint32_t)(node)->header[3])

#define checksum_set(node, val) (\
	(node)->header[0] = (unsigned char)((val) >> 24),\
	(node)->header[1] = (unsigned char)((val) >> 16),\
	(node)->header[2] = (unsigned char)((val) >> 8),\
	(node)->header[3] = (unsigned char)((val)))

#define type_get(n) ((n)->header[4])
#define type_set(n, val) ((n)->header[4] = (val))
#define length_get(n) ((n)->header[6] << 8 | (n)->header[7])
#define length_set(n, val) ((n)->header[6] = (unsigned char)((val) >> 8), (n)->header[7] = (unsigned char)(val))
#define parent_get(n) (node*)ptroff_get(tree->root, (n)->header + 8)
#define parent_set(n, val) ptroff_set(tree->root, (n)->header + 8, val)
#define next_get(n) (node*)ptroff_get(tree->root, (n)->header + 16)
#define next_set(n, val) ptroff_set(tree->root, (n)->header + 16, val)
#define prev_get(n) (node*)ptroff_get(tree->root, (n)->header + 24)
#define prev_set(n, val) ptroff_set(tree->root, (n)->header + 24, val)

#define key_get(n, index) (\
	assert_fatal(CLOD_DEBUG, index >= 0 && index < length_get(n) && index < CAPACITY(n),\
		"Key index %i out of bounds for node %ptr with %i (maximum %i) keys.",\
		index, (void*)n, length_get(n), CAPACITY(n)),\
	((n)->keys + (index) * KEY_SIZE))

#define key_set(n, index, val) \
	assert_fatal(CLOD_DEBUG, index >= 0 && index < CAPACITY(n),\
		"Key index %i out of bounds for node %ptr with maximum %i keys.",\
		index, (void*)n, CAPACITY(n)),\
	copy(((n)->keys + (index) * KEY_SIZE), (val), KEY_SIZE);

#define val_get(n, index) (\
	assert_fatal(CLOD_DEBUG, index >= 0 && index < length_get(n) && index < LEAF_CAPACITY,\
		"Value index %i out of bounds for node %ptr with %i keys.",\
		index, (void*)n, length_get(n)),\
	((n)->keys + LEAF_CAPACITY * KEY_SIZE + (index) * VAL_SIZE))

#define val_set(n, index, val) (\
	assert_fatal(CLOD_DEBUG, index >= 0 && index < LEAF_CAPACITY,\
		"Value index %i out of bounds for node %ptr with maximum %i values.",\
		index, (void*)n, LEAF_CAPACITY),\
	copy(((n)->keys + LEAF_CAPACITY * KEY_SIZE + (index) * VAL_SIZE), (val), VAL_SIZE))

#define branch_get(n, index) (\
	assert_fatal(CLOD_DEBUG, index >= 0 && index <= length_get(n) && index <= INTL_CAPACITY,\
		"Branch index %i out of bounds for node %ptr with %i keys.",\
		index, (void*)n, length_get(n)),\
	(node*)ptroff_get(tree->root, (unsigned char*)(n)->keys + tree->node_size - (index) * 8 - 8 - 32))

#define branch_set(n, index, val) (\
	assert_fatal(CLOD_DEBUG, index >= 0 && index <= INTL_CAPACITY,\
		"Branch index %i out of bounds for node %ptr with maximum %i branches.",\
		index, (void*)n, INTL_CAPACITY + 1),\
	assert_fatal(CLOD_DEBUG, val == nullptr || type_get((node*)val) == TYPE_LEAF || type_get((node*)val) == TYPE_INTL,\
		"Inserting invalid node %ptr into index %i of branch %ptr",\
		(void*)val, index, (void*)n),\
	ptroff_set(tree->root, (unsigned char*)(n)->keys + tree->node_size - (index) * 8 - 8 - 32, val))

#define as_int64(ptr, size) (\
	(size) >= sizeof(int64_t) ? (int64_t)*(int64_t*)(ptr) :\
	(size) >= sizeof(int32_t) ? (int64_t)*(int32_t*)(ptr) :\
	(size) >= sizeof(int16_t) ? (int64_t)*(int16_t*)(ptr) :\
	(size) >= sizeof(int8_t)  ? (int64_t)*(uint8_t*)(ptr) :\
	INT64_C(0)\
)

#if CLOD_DEBUG
CLOD_API
void clod_tree_debug_print(const struct clod_tree *tree, const node *n, const int depth) {
	if (depth == 0)
		clod_stream_format(clod_stderr, CLOD_STRING_C("\n==== Begin Tree Debug ====\n"));

	for (int i = 0; i < depth; i++)
		clod_stream_format(clod_stderr, CLOD_STRING_C("\t"));

	if (depth > 8) {
		clod_stream_format(clod_stderr, CLOD_STRING_C("Tree is too deep to print. Skipping...\n"));
		return;
	}

	if (n == nullptr) {
		clod_stream_format(clod_stderr, CLOD_STRING_C("NULL\n"));
		return;
	}

	if (((uintptr_t)n & 0xFFFF700000000000ull) != 0x0000700000000000ull) {
		clod_stream_format(clod_stderr, CLOD_STRING_C("Node %ptr is not a valid node!\n"), (void*)n);
		return;
	}

	if (type_get(n) == TYPE_LEAF) {
		clod_stream_format(clod_stderr, CLOD_STRING_C("Leaf %ptr (Parent: %ptr, Next: %ptr, Prev: %ptr, Keys: %i)\n"),
			(void*)n, (void*)parent_get(n), (void*)next_get(n), (void*)prev_get(n), length_get(n));
		if (length_get(n)) {
			for (int i = 0; i <= depth; i++)
				clod_stream_format(clod_stderr, CLOD_STRING_C("\t"));
			for (int i = 0; i < length_get(n); i++) {
				clod_stream_format(clod_stderr, CLOD_STRING_C("| [%u]: %i -> %i "), i, *(int*)key_get(n, i), *(int*)val_get(n, i));
			}
			clod_stream_format(clod_stderr, CLOD_STRING_C("|\n"));
		}
	} else {
		clod_stream_format(clod_stderr, CLOD_STRING_C("Intl %ptr (Parent: %ptr, Keys: %i)\n"),
			(void*)n, (void*)parent_get(n), length_get(n));

		for (int i = 0; i <= depth; i++)
			clod_stream_format(clod_stderr, CLOD_STRING_C("\t"));



		for (int i = 0; i < length_get(n); i++) {
			clod_stream_format(clod_stderr, CLOD_STRING_C("| Branch: %ptr | Key: %i "), branch_get(n, i), *(int*)key_get(n, i));
		}
		clod_stream_format(clod_stderr, CLOD_STRING_C("| Branch: %ptr |\n"), branch_get(n, length_get(n)));


		for (int i = 0; i < length_get(n); i++) {
			clod_tree_debug_print(tree, branch_get(n, i), depth + 1);

			for (int k = 0; k < depth; k++)
				clod_stream_format(clod_stderr, CLOD_STRING_C("\t"));
			clod_stream_format(clod_stderr, CLOD_STRING_C("Key: %i\n"), *(int*)key_get(n, i));
		}

		clod_tree_debug_print(tree, branch_get(n, length_get(n)), depth + 1);
	}

	if (depth == 0)
		clod_stream_format(clod_stderr, CLOD_STRING_C("==== End Tree Debug ====\n\n"));
}
#endif

bool leaf_validate(
	const struct clod_tree *tree,
	const node *n,
	const node *parent,
	const void **prev_key,
	bool *prev_key_was_last_leaf_key,
	const node **prev_leaf,
	const int depth
) {
	bool ok = true;

	if (depth != 0) {
		debug(CLOD_DEBUG, "Leaf node %ptr is at the wrong depth: %i from where it should be.", (void*)n, depth);
		ok = false;
	}

	if (prev_get(n) != *prev_leaf) {
		debug(CLOD_DEBUG, "Previous field on leaf node %ptr points to %ptr, but the actual previous leaf node is %ptr.",
			(void*)n, prev_get(n), (void*)*prev_leaf);

		ok = false;
	}

	if (*prev_leaf && next_get(*prev_leaf) != n) {
		debug(CLOD_DEBUG, "Next field of leaf node %ptr points to %ptr, but the actual next leaf node is %ptr.",
			(void*)*prev_leaf, next_get(*prev_leaf), (void*)n);

		ok = false;
	}

	if (parent_get(n) != parent) {
		debug(CLOD_DEBUG, "Parent field of leaf node %ptr points to %ptr, but the actual parent node is %ptr.",
			(void*)n, parent_get(n), (void*)parent);

		ok = false;
	}

	if (parent_get(n) && length_get(n) < LEAF_CAPACITY / 2) {
		debug(CLOD_DEBUG, "Leaf node %ptr has %i keys which is less than half the capacity (%i out of %i total) required for a balanced tree.",
			(void*)n, length_get(n), LEAF_CAPACITY / 2, LEAF_CAPACITY);
		ok = false;
	}

	if (length_get(n) > LEAF_CAPACITY) {
		debug(CLOD_DEBUG, "Leaf node %ptr has %i keys, which is more than the capacity (%i).",
			(void*)n, length_get(n), LEAF_CAPACITY);
		ok = false;
	}

	for (int i = 0; i < length_get(n); i++) {
		if (*prev_key) {
			if (tree->compare(key_get(n, i), *prev_key, tree->compare_user) <= 0) {
				debug(CLOD_DEBUG, "Key %xi64 from index %xi64 in node %ptr is out of order with previous key %xi64.",
					as_int64(key_get(n, i), tree->key_size), i, (void*)n, as_int64(*prev_key, tree->key_size));

				ok = false;
			}
		}

		*prev_key = key_get(n, i);
		if (i == length_get(n) - 1) {
			*prev_key_was_last_leaf_key = true;
		} else {
			*prev_key_was_last_leaf_key = false;
		}
	}

	*prev_leaf = n;
	return ok;
}

bool intl_validate(
	const struct clod_tree *tree,
	const node *n,
	const node *parent,
	const void **prev_key,
	bool *prev_key_was_last_leaf_key,
	const node **prev_leaf,
	const int depth
) {
	bool ok = true;

	if (parent_get(n) != parent) {
		debug(CLOD_DEBUG, "Parent field of internal node %ptr points to %ptr, but the actual parent node is %ptr.",
			(void*)n, parent_get(n), (void*)parent);

		ok = false;
	}

	if (parent_get(n) && length_get(n) < INTL_CAPACITY / 2) {
		debug(CLOD_DEBUG, "Internal node %ptr has %i keys which is less than half the capacity (%i out of %i total) required for a balanced tree.",
			(void*)n, length_get(n), INTL_CAPACITY / 2, INTL_CAPACITY);
		ok = false;
	}

	if (length_get(n) > INTL_CAPACITY) {
		debug(CLOD_DEBUG, "Internal node %ptr has %i keys, which is more than the capacity (%i).",
			(void*)n, length_get(n), INTL_CAPACITY);
		ok = false;
	}

	for (int i = 0; i < length_get(n); i++) {
		const node *branch = branch_get(n, i);
		if (!branch) {
			debug(CLOD_DEBUG, "Branch %i of internal node %ptr is null.", i, (void*)n);
			ok = false;
		} else {
			if (type_get(branch) == TYPE_LEAF) {
				if (!leaf_validate(tree, branch, n, prev_key, prev_key_was_last_leaf_key, prev_leaf, depth + 1)) {
					ok = false;
				}
			} else if (type_get(branch) == TYPE_INTL) {
				if (!intl_validate(tree, branch, n, prev_key, prev_key_was_last_leaf_key, prev_leaf, depth + 1)) {
					ok = false;
				}
			} else {
				debug(CLOD_DEBUG, "Branch %i of internal node %ptr has invalid type %i.", i, (void*)n, type_get(branch));
				ok = false;
			}
		}

		if (*prev_key) {
			const int cmp = tree->compare(key_get(n, i), *prev_key, tree->compare_user);
			if (*prev_key_was_last_leaf_key ? cmp < 0 : cmp <= 0) {
				debug(CLOD_DEBUG, "Key %xi64 from index %xi64 in internal node %ptr is out of order with previous key %xi64.",
					as_int64(key_get(n, i), tree->key_size), i, (void*)n, as_int64(*prev_key, tree->key_size));
				ok = false;
			}
		}

		*prev_key = key_get(n, i);
		*prev_key_was_last_leaf_key = false;
	}

	const node *branch = branch_get(n, length_get(n));
	if (!branch) {
		debug(CLOD_DEBUG, "Branch %i of internal node %ptr is null.", length_get(n), (void*)n);
		ok = false;
	} else {
		if (type_get(branch) == TYPE_LEAF) {
			if (!leaf_validate(tree, branch, n, prev_key, prev_key_was_last_leaf_key, prev_leaf, depth + 1)) {
				ok = false;
			}
		} else if (type_get(branch) == TYPE_INTL) {
			if (!intl_validate(tree, branch, n, prev_key, prev_key_was_last_leaf_key, prev_leaf, depth + 1)) {
				ok = false;
			}
		} else {
			debug(CLOD_DEBUG, "Branch %i of internal node %ptr has invalid type %i.", length_get(n), (void*)n, type_get(branch));
			ok = false;
		}
	}

	return ok;
}

bool clod_tree_validate(const struct clod_tree *tree) {
	const void *prev_key = nullptr;
	bool prev_key_was_leaf = false;
	const node *prev_leaf = nullptr;

	if (tree->root == nullptr) return true;

	int depth = 0;
	for (node *n = tree->root; n && type_get(n) == TYPE_INTL; n = branch_get(n, 0)) {
		depth++;
	}

	bool ok;
	if (type_get(tree->root) == TYPE_INTL) {
		ok = intl_validate(tree, tree->root, nullptr, &prev_key, &prev_key_was_leaf, &prev_leaf, -depth);
	} else if (type_get(tree->root) == TYPE_LEAF) {
		ok = leaf_validate(tree, tree->root, nullptr, &prev_key, &prev_key_was_leaf, &prev_leaf, -depth);
	} else {
		debug(CLOD_DEBUG, "Root node of tree %ptr has invalid type %i.", (void*)tree->root, type_get(tree->root));
		return false;
	}

	#if CLOD_DEBUG
	if (!ok ) {
		clod_tree_debug_print(tree, tree->root, 0);
	}
	#endif

	return ok;
}

/// Return the index of the key in \p n which compares equal to, or the closest value higher than, key.
/// @return true if an exact match was found.
static bool search(const struct clod_tree *tree, const node *n, const void *key, int *index) {
	int l = 0, h = length_get(n);
	while (l < h) {
		const int m = (l + h) / 2;
		const int cmp = tree->compare(key_get(n, m), key, tree->compare_user);
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
static node *parent_branch_get(const struct clod_tree *tree, const node *n, int *parent_index) {
	node *parent = parent_get(n);
	if (!parent) {
		return nullptr;
	}

	for (int i = 0; i <= length_get(parent); i++) {
		if (branch_get(parent, i) == n) {
			if (parent_index) *parent_index = i;
			return parent;
		}
	}

	assert_fatal(CLOD_DEBUG, false, "Parent of %ptr does not contain it! Invalid tree.", (void*)n);
}

/// Insert a key and value into the given leaf node,
static void leaf_insert(const struct clod_tree *tree, node *n, int index, const void *key, const void *val) {
	const int len = length_get(n);
	assert_fatal(CLOD_DEBUG, index <= len,
		"Index %i out of bounds for inserting into node %ptr with %i keys.",
		index, (void*)n, len);

	for (int i = len; i > index; i--) key_set(n, i, key_get(n, i - 1));
	for (int i = len; i > index; i--) val_set(n, i, val_get(n, i - 1));

	length_set(n, len + 1);
	key_set(n, index, key);
	val_set(n, index, val);

	if (index != len) {
		return;
	}

	node *parent = parent_branch_get(tree, n, &index);
	while (parent && index == length_get(parent)) {
		parent = parent_branch_get(tree, parent, &index);
	}

	if (parent) {
		key_set(parent, index, key_get(n, len));
	}
}

/// Insert a key and branch into the given internal node, with the branch being placed to the left of the new key.
static void intl_insert_left(const struct clod_tree *tree, node *n, const int index, const void *key, node *branch) {
	for (int i = length_get(n) + 1; i > index; i--) branch_set(n, i, branch_get(n, i - 1));
	for (int i = length_get(n); i > index; i--) key_set(n, i, key_get(n, i - 1));

	length_set(n, length_get(n) + 1);
	key_set(n, index, key);
	branch_set(n, index, branch);
	parent_set(branch, n);
}

/// Insert a key and branch into the given internal node, with the branch being placed to the right of the new key.
static void intl_insert_right(const struct clod_tree *tree, node *n, const int index, const void *key, node *branch) {
	for (int i = length_get(n) + 1; i > index + 1; i--) branch_set(n, i, branch_get(n, i - 1));
	for (int i = length_get(n); i > index; i--) key_set(n, i, key_get(n, i - 1));

	length_set(n, length_get(n) + 1);
	key_set(n, index, key);
	branch_set(n, index + 1, branch);
	parent_set(branch, n);
}

/// Remove the element in the given leaf node, and remove the
/// instance of its key in the tree if one exists.
static void leaf_remove(const struct clod_tree *tree, node *const n, int index) {
	const int len = length_get(n);
	for (int i = index; i < len - 1; i++) val_set(n, i, val_get(n, i + 1));
	for (int i = index; i < len - 1; i++) key_set(n, i, key_get(n, i + 1));
	length_set(n, len - 1);

	if (index != len - 1) {
		return;
	}

	node *parent = parent_branch_get(tree, n, &index);
	while (parent && index == length_get(parent)) {
		parent = parent_branch_get(tree, parent, &index);
	}

	if (parent) {
		key_set(parent, index, key_get(n, len - 2));
	}
}

/// Remove a key and branch from the given internal node, with the branch to the left of the key being removed.
static void intl_remove_left(const struct clod_tree *tree, node *const n, const int index) {
	for (int i = index; i < length_get(n); i++) branch_set(n, i, branch_get(n, i + 1));
	for (int i = index; i < length_get(n) - 1; i++) key_set(n, i, key_get(n, i + 1));
	length_set(n, length_get(n) - 1);
}

/// Remove a key and branch from the given internal node, with the branch to the right of the key being removed.
static void intl_remove_right(const struct clod_tree *tree, node *const n, const int index) {
	for (int i = index + 1; i < length_get(n); i++) branch_set(n, i, branch_get(n, i + 1));
	for (int i = index; i < length_get(n) - 1; i++) key_set(n, i, key_get(n, i + 1));
	length_set(n, length_get(n) - 1);
}

/// Split the elements in \p n between \p left and \p right.
/// @return index in \p n of the promoted key.
static int leaf_split(const struct clod_tree *tree, const node *n, node *left, node *right) {
	const int keys = length_get(n);
	const int left_size = (keys + 1) / 2;
	const int right_size = keys - left_size;

	assert_fatal(CLOD_DEBUG, keys >= LEAF_CAPACITY / 2,
		"Refusing to split leaf node %ptr with %i keys. Nodes may never be less than half full.",
		(void*)n, keys);

	int parent_index;
	node *const parent = parent_branch_get(tree, n, &parent_index);
	assert_fatal(CLOD_DEBUG, !parent || length_get(parent) < INTL_CAPACITY,
		"Cannot split leaf node %ptr when its parent %ptr has %i out of %i keys occupied.",
		(void*)n, (void*)parent, length_get(parent), INTL_CAPACITY);

	type_set(left, TYPE_LEAF);
	type_set(right, TYPE_LEAF);
	parent_set(left, parent);
	parent_set(right, parent);
	const node *n_next = next_get(n), *n_prev = prev_get(n);
	next_set(left, right);
	next_set(right, n_next);
	prev_set(left, n_prev);
	prev_set(right, left);

	if (parent) {
		branch_set(parent, parent_index, left);
		intl_insert_right(tree, parent, parent_index, key_get(n, left_size - 1), right);
	}

	if (n == left) {
		for (int i = 0; i < right_size; i++) key_set(right, i, key_get(n, i + left_size));
		for (int i = 0; i < right_size; i++) val_set(right, i, val_get(n, i + left_size));
	} else {
		for (int i = 0; i < left_size; i++) key_set(left, i, key_get(n, i));
		for (int i = 0; i < right_size; i++) key_set(right, i, key_get(n, i + left_size));
		for (int i = 0; i < left_size; i++) val_set(left, i, val_get(n, i));
		for (int i = 0; i < right_size; i++) val_set(right, i, val_get(n, i + left_size));
	}

	length_set(left, left_size);
	length_set(right, right_size);
	return left_size - 1;
}

/// Split the elements in \p n between \p left and \p right.
/// @return index in \p n of the promoted key.
static int intl_split(const struct clod_tree *tree, const node *n, node *left, node *right) {
	const int keys = length_get(n);
	const int left_size = keys / 2;
	const int right_size = keys - left_size - 1;

	assert_fatal(CLOD_DEBUG, keys >= INTL_CAPACITY / 2,
		"Refusing to split internal node %ptr with %i keys. Nodes may never be less than half full.",
		(void*)n, keys);

	int parent_index;
	node *const parent = parent_branch_get(tree, n, &parent_index);
	assert_fatal(CLOD_DEBUG, !parent || length_get(parent) < INTL_CAPACITY,
		"Cannot split leaf node when its parent %ptr has no space.", (void*)parent);

	type_set(left, TYPE_INTL);
	type_set(right, TYPE_INTL);
	parent_set(left, parent);
	parent_set(right, parent);
	if (parent) {
		branch_set(parent, parent_index, left);
		intl_insert_right(tree, parent, parent_index, key_get(n, left_size), right);
	}

	if (n == left) {
		for (int i = 0; i < right_size; i++) key_set(right, i, key_get(n, i + left_size + 1));
		for (int i = 0; i <= right_size; i++) branch_set(right, i, branch_get(n, i + left_size + 1));
	} else {
		for (int i = 0; i < left_size; i++) key_set(left, i, key_get(n, i));
		for (int i = 0; i < right_size; i++) key_set(right, i, key_get(n, i + left_size + 1));
		for (int i = 0; i <= left_size; i++) branch_set(left, i, branch_get(n, i));
		for (int i = 0; i <= right_size; i++) branch_set(right, i, branch_get(n, i + left_size + 1));
	}

	length_set(left, left_size);
	length_set(right, right_size);
	for (int i = 0; i <= left_size; i++) parent_set(branch_get(left, i), left);
	for (int i = 0; i <= right_size; i++) parent_set(branch_get(right, i), right);

	return left_size;
}

/// Copies the elements from \p left and \p right into \p n.
static void leaf_merge(const struct clod_tree *tree, node *n, const node *left, const node *right) {
	const int left_len = length_get(left);
	const int right_len = length_get(right);

	assert_fatal(CLOD_DEBUG, left_len + right_len <= LEAF_CAPACITY,
		"Cannot merge leaf nodes %ptr and %ptr because their combined elements %i + %i = %i is larger than the maximum %i.",
		(void*)left, (void*)right, left_len, right_len, left_len + right_len, LEAF_CAPACITY);

	int parent_index;
	node *parent = parent_branch_get(tree, left, &parent_index);
	assert_fatal(CLOD_DEBUG, parent == parent_get(right),
		"Cannot merge leaf nodes %ptr and %ptr that belong to different parents %ptr and %ptr.",
		(void*)left, (void*)right, (void*)parent_get(left), (void*)parent_get(right));

	type_set(n, TYPE_LEAF);
	length_set(n, left_len + right_len);
	parent_set(n, parent);
	if (prev_get(left)) next_set(prev_get(left), n);
	if (next_get(right)) prev_set(next_get(right), n);
	next_set(n, next_get(right));
	prev_set(n, prev_get(left));

	intl_remove_right(tree, parent, parent_index);
	branch_set(parent, parent_index, n);

	if (n == left) {
		for (int i = 0; i < right_len; i++) key_set(n, i + left_len, key_get(right, i));
		for (int i = 0; i < right_len; i++) val_set(n, i + left_len, val_get(right, i));
	} else {
		for (int i = right_len - 1; i >= 0; i--) key_set(n, i + left_len, key_get(right, i));
		for (int i = 0; i < left_len; i++) key_set(n, i, key_get(left, i));
		for (int i = right_len - 1; i >= 0; i--) val_set(n, i + left_len, val_get(right, i));
		for (int i = 0; i < left_len; i++) val_set(n, i, val_get(left, i));
	}
}

/// Copies the elements from \p left and \p right into \p n.
/// @return index in \p n of the demoted key.
static int intl_merge(const struct clod_tree *tree, node *n, const node *left, const node *right) {
	const int left_len = length_get(left);
	const int right_len = length_get(right);

	assert_fatal(CLOD_DEBUG, left_len + right_len + 1 <= INTL_CAPACITY,
		"Cannot merge internal nodes %ptr and %ptr because their combined elements %i + %i = %i is larger than the maximum %i.",
		(void*)left, (void*)right, left_len, right_len, left_len + right_len, INTL_CAPACITY);

	int parent_index;
	node *parent = parent_branch_get(tree, left, &parent_index);
	assert_fatal(CLOD_DEBUG, parent == parent_get(right),
		"Cannot merge internal nodes %ptr and %ptr that belong to different parents %ptr and %ptr.",
		(void*)left, (void*)right, (void*)parent_get(left), (void*)parent_get(right));

	type_set(n, TYPE_INTL);
	length_set(n, left_len + right_len + 1);
	parent_set(n, parent);

	if (n == left) {
		for (int i = 0; i < right_len; i++) key_set(n, i + left_len + 1, key_get(right, i));
		for (int i = 0; i <= right_len; i++) branch_set(n, i + left_len + 1, branch_get(right, i));
	} else {
		for (int i = right_len - 1; i >= 0; i--) key_set(n, i + left_len + 1, key_get(right, i));
		for (int i = 0; i < left_len; i++) key_set(n, i, key_get(left, i));
		for (int i = right_len - 1; i >= 0; i--) branch_set(n, i + left_len + 1, branch_get(right, i));
		for (int i = 0; i <= left_len; i++) branch_set(n, i, branch_get(left, i));
	}

	key_set(n, left_len, key_get(parent, parent_index));
	for (int i = 0; i <= length_get(n); i++) parent_set(branch_get(n, i), n);
	intl_remove_right(tree, parent, parent_index);
	branch_set(parent, parent_index, n);

	return left_len;
}

/// Balance the given node.
static void balance(const struct clod_tree *tree, node *n) {
	int index;
	node *parent = parent_branch_get(tree, n, &index);
	if (!parent) {
		debug(CLOD_DEBUG, "Cannot balance root node.");
		return;
	}

	node *left, *right;
	if (index == 0) {
		left = n;
		right = branch_get(parent, 1);
	} else {
		left = branch_get(parent, index - 1);
		right = n;
	}
	const int left_len = length_get(left), right_len = length_get(right);

	if (type_get(n) == TYPE_LEAF) {
		if (left_len + right_len <= LEAF_CAPACITY) {

			leaf_merge(tree, left, left, right);
			tree->allocator.free(right, tree->allocator.self);


		} else if (left_len < right_len) {

			leaf_insert(tree, left, left_len, key_get(right, 0), val_get(right, 0));
			leaf_remove(tree, right, 0);

		} else if (right_len < left_len) {

			leaf_insert(tree, right, 0, key_get(left, left_len - 1), val_get(left, left_len - 1));
			leaf_remove(tree, left, left_len - 1);

		}
	} else if (type_get(n) == TYPE_INTL) {

		if (left_len + right_len + 1 <= INTL_CAPACITY) {

			intl_merge(tree, left, left, right);
			tree->allocator.free(right, tree->allocator.self);

		} else if (left_len < right_len) {

			intl_insert_right(tree, left, left_len, key_get(parent, index), branch_get(right, 0));
			key_set(parent, index, key_get(right, 0));
			intl_remove_left(tree, right, 0);

		} else if (right_len < left_len) {

			intl_insert_left(tree, right, 0, key_get(parent, index), branch_get(left, left_len - 1));
			key_set(parent, index, key_get(left, left_len - 2));
			intl_remove_right(tree, left, left_len - 1);

		}

	}
}

bool clod_tree_create(struct clod_tree *tree) {
	if (tree->key_size == 0) {
		debug(CLOD_DEBUG, "Tree key size cannot be zero.");
		return false;
	}

	const size_t min_node_size = sizeof(node) + tree->key_size + (tree->val_size < 16 ? 16 : tree->val_size);
	if (tree->node_size < min_node_size) {
		debug(CLOD_DEBUG, "Tree node size of %i is too small. Need %i for a key and value size of %i and %i.",
			tree->node_size, min_node_size, tree->key_size, tree->val_size);
		return false;
	}

	if (!tree->compare) {
		debug(CLOD_DEBUG, "Tree compare function cannot be null.");
		return false;
	}

	if (!tree->root) {
		if (!tree->allocator.allocate || !tree->allocator.free) {
			debug(CLOD_DEBUG, "Tree allocator methods cannot be null.");
			return false;
		}

		tree->root = tree->allocator.allocate(tree->node_size, tree->allocator.self);
		if (!tree->root) {
			debug(CLOD_DEBUG, "Failed to allocate root node.");
			return false;
		}

		type_set(tree->root, TYPE_LEAF);
		length_set(tree->root, 0);
		parent_set(tree->root, nullptr);
	}

	return true;
}

void clod_tree_destroy(struct clod_tree *tree) {
	if (!tree->root) return;
	node *n = tree->root;
	while (n) {
		const int len = length_get(n);

		if (
			type_get(n) == TYPE_LEAF ||
			(length_get(n) == 0 && branch_get(n, 0) == nullptr)
		) {
			node *tmp = parent_get(n);
			tree->allocator.free(n, tree->allocator.self);
			n = tmp;
			continue;
		}

		node *next = branch_get(n, len);
		branch_set(n, len, nullptr);
		if (len > 0) {
			length_set(n, len - 1);
		}
		n = next;
	}

	tree->root = nullptr;
}

bool clod_tree_add(const struct clod_tree *tree, const void *key, const void *val, void *key_out, void *val_out) {
	node *n = tree->root;
	int index;

	while (type_get(n) == TYPE_INTL) {
		search(tree, n, key, &index);
		n = branch_get(n, index);
	}

	if (search(tree, n, key, &index)) {
		if (key_out) copy(key_out, key_get(n, index), tree->key_size);
		if (val_out) copy(val_out, val_get(n, index), tree->val_size);
		return false;
	}

	if (length_get(n) < LEAF_CAPACITY) {
		leaf_insert(tree, n, index, key, val);
		return true;
	}

	int parent_index;
	node *parent = parent_branch_get(tree, n, &parent_index);
	if (parent && parent_index > 0 && length_get(branch_get(parent, parent_index - 1)) < LEAF_CAPACITY) {
		node *left = branch_get(parent, parent_index - 1);
		if (index == 0) {
			leaf_insert(tree, left, length_get(left), key, val);
		} else {
			leaf_insert(tree, left, length_get(left), key_get(n, 0), val_get(n, 0));
			leaf_remove(tree, n, 0);
			leaf_insert(tree, n, index - 1, key, val);
		}

		return true;
	}

	if (parent && parent_index < length_get(parent) && length_get(branch_get(parent, parent_index + 1)) < LEAF_CAPACITY) {
		node *right = branch_get(parent, parent_index + 1);

		leaf_insert(tree, right, 0, key_get(n, length_get(n)), val_get(n, length_get(n)));
		leaf_remove(tree, n, length_get(n));
		leaf_insert(tree, n, index, key, val);

		return true;
	}

	while (parent_get(n) && length_get(parent_get(n)) >= INTL_CAPACITY) {
		parent = parent_branch_get(tree, n, &parent_index);

		if (parent_index > 0 && length_get(branch_get(parent, parent_index - 1)) < INTL_CAPACITY) {
			node *left = branch_get(parent, parent_index - 1);
			intl_insert_right(tree, left, length_get(left), key_get(parent, parent_index - 1), branch_get(n, 0));
			key_set(parent, parent_index - 1, key_get(n, 0));
			intl_remove_left(tree, n, 0);

			search(tree, parent, key, &index);
			n = branch_get(parent, index);
			search(tree, n, key, &index);
			n = branch_get(n, index);
			break;
		}

		if (parent_index < length_get(parent) - 1 && length_get(branch_get(parent, parent_index + 1)) < INTL_CAPACITY) {
			node *right = branch_get(parent, parent_index + 1);
			intl_insert_left(tree, right, 0, key_get(parent, parent_index), branch_get(n, length_get(n)));
			key_set(parent, parent_index, key_get(n, length_get(n) - 1));
			intl_remove_right(tree, n, length_get(n) - 1);

			search(tree, parent, key, &index);
			n = branch_get(parent, index);
			search(tree, n, key, &index);
			n = branch_get(n, index);
			break;
		}

		n = parent_get(n);
	}

	if (parent_get(n) == nullptr) {
		node *left = tree->allocator.allocate(tree->node_size, tree->allocator.self);
		node *right = tree->allocator.allocate(tree->node_size, tree->allocator.self);
		if (!left || !right) {
			debug(CLOD_DEBUG, "Failed to allocate new internal node.");
			if (left) tree->allocator.free(left, tree->allocator.self);
			if (right) tree->allocator.free(right, tree->allocator.self);
			return false;
		}

		if (type_get(n) == TYPE_LEAF) {
			const int split = leaf_split(tree, n, left, right);

			key_set(n, 0, key_get(n, split));
			type_set(n, TYPE_INTL);
			length_set(n, 1);
			branch_set(n, 0, left);
			branch_set(n, 1, right);
			parent_set(left, n);
			parent_set(right, n);

			search(tree, n, key, &index);
			n = branch_get(n, index);
			search(tree, n, key, &index);
			leaf_insert(tree, n, index, key, val);
			return true;
		}

		if (type_get(n) == TYPE_INTL) {
			const int split = intl_split(tree, n, left, right);

			key_set(n, 0, key_get(n, split));
			type_set(n, TYPE_INTL);
			length_set(n, 1);
			branch_set(n, 0, left);
			branch_set(n, 1, right);
			parent_set(left, n);
			parent_set(right, n);

			search(tree, n, key, &index);
			n = branch_get(n, index);
			search(tree, n, key, &index);
			n = branch_get(n, index);
		}
	}

	while (type_get(n) == TYPE_INTL) {
		node *new = tree->allocator.allocate(tree->node_size, tree->allocator.self);
		if (!new) {
			debug(CLOD_DEBUG, "Failed to allocate new internal node.");
			return false;
		}

		search(tree, n, key, &index);
		const int split = intl_split(tree, n, n, new);
		if (index < split) {
			n = branch_get(n, index);
		} else {
			n = branch_get(new, index - split - 1);
		}
	}

	node *new = tree->allocator.allocate(tree->node_size, tree->allocator.self);
	if (!new) {
		debug(CLOD_DEBUG, "Failed to allocate new leaf node.");
		return false;
	}

	search(tree, n, key, &index);
	const int split = leaf_split(tree, n, n, new);
	if (index >= split) {
		n = new;
		index = index - split - 1;
	}

	leaf_insert(tree, n, index, key, val);
	return true;
}

bool clod_tree_get(const struct clod_tree *tree, const void *key, void *key_out, void *val_out) {
	const node *n = tree->root;
	int index;
	while (type_get(n) == TYPE_INTL) {
		search(tree, n, key, &index);
		n = branch_get(n, index);
	}

	if (search(tree, n, key, &index)) {
		if (key_out) copy(key_out, key_get(n, index), tree->key_size);
		if (val_out) copy(val_out, val_get(n, index), tree->val_size);
		return true;
	}
	return false;
}

bool clod_tree_del(const struct clod_tree *tree, const void *key, void *key_out, void *val_out) {
	node *n = tree->root;
	int index;
	while (type_get(n) == TYPE_INTL) {
		search(tree, n, key, &index);
		n = branch_get(n, index);
	}

	if (!search(tree, n, key, &index)) {
		return false;
	}

	if (key_out) copy(key_out, key_get(n, index), tree->key_size);
	if (val_out) copy(val_out, val_get(n, index), tree->val_size);
	leaf_remove(tree, n, index);

	while (n && parent_get(n) && length_get(n) < LEAF_CAPACITY / 2) {
		balance(tree, n);
		n = parent_get(n);
	}

	return true;
}
