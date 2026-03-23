#include <strings.h>

#include "clod_config.h"
#include <clod/structures/tree.h>
#include "clod/debug.h"

#define HEADER_SIZE 32
#define TYPE_LEAF 0
#define TYPE_INTL 1

// TODO: use memory copy method (clod/string.h?) instead of manual copy loops everywhere.

static unsigned short tree_leaf_keys_max(const struct clod_tree *tree) {
	return (tree->node_size - HEADER_SIZE) / (tree->key_size + tree->val_size);
}
static unsigned short tree_intl_keys_max(const struct clod_tree *tree) {
	return (tree->node_size - HEADER_SIZE - 8) / (tree->key_size + 8);
}

typedef struct clod_tree_node {
	unsigned char header[32];
} node;

static void *ptr(const struct clod_tree *tree, const unsigned char *data) {
	ptrdiff_t offset = (ptrdiff_t)(
		(uint64_t)data[0] << 56 | (uint64_t)data[1] << 48 |
		(uint64_t)data[2] << 40 | (uint64_t)data[3] << 32 |
		(uint64_t)data[4] << 24 | (uint64_t)data[5] << 16 |
		(uint64_t)data[6] << 8  | (uint64_t)data[7]);
	if (offset == 0) return nullptr;
	return (node*)((char*)tree->root + offset - 1);
}
static void ptr_set(const struct clod_tree *tree, unsigned char *data, const void *ptr) {
	uint64_t offset;
	if (ptr) {
		offset = (uint64_t)((char*)ptr - (char*)tree->root) + 1;
	} else {
		offset = 0;
	}
	data[0] = (unsigned char)(offset >> 56); data[1] = (unsigned char)(offset >> 48);
	data[2] = (unsigned char)(offset >> 40); data[3] = (unsigned char)(offset >> 32);
	data[4] = (unsigned char)(offset >> 24); data[5] = (unsigned char)(offset >> 16);
	data[6] = (unsigned char)(offset >> 8 ); data[7] = (unsigned char)offset;
}

static uint32_t node_checksum(const node *n) {
	return
		(uint32_t)n->header[0] << 24 | (uint32_t)n->header[1] << 16 |
		(uint32_t)n->header[2] << 8  | (uint32_t)n->header[3];
}
static void node_checksum_set(node *n, const uint32_t checksum) {
	n->header[0] = (unsigned char)(checksum >> 24);
	n->header[1] = (unsigned char)(checksum >> 16);
	n->header[2] = (unsigned char)(checksum >> 8);
	n->header[3] = (unsigned char)checksum;
}
static unsigned char node_type(const node *n) {
	return n->header[4];
}
static void node_type_set(node *n, const unsigned char type) {
	n->header[4] = type;
}
static node *node_parent(const struct clod_tree *tree, const node *n) { return ptr(tree, (unsigned char*)n + 8); }
static void node_parent_set(const struct clod_tree *tree, node *n, const node *parent) { ptr_set(tree, (unsigned char*)n + 8, parent); }

static unsigned short leaf_num_keys(const struct clod_tree *tree, const node *n) {
	unsigned short keys = (unsigned short)(n->header[6] << 8) | n->header[7];
	assert_fatal(CLOD_MEMORY_CHECK, keys <= tree_leaf_keys_max(tree),
		"Invalid key count %i in node %ptr for tree with only %i keys per leaf node.",
		keys, (void*)n, tree_leaf_keys_max(tree));
	assert_fatal(CLOD_DEBUG, !node_parent(tree, n) || keys >= tree_leaf_keys_max(tree) / 2,
		"Too few keys %i in node %ptr for tree with minimum %i keys per leaf node.",
		keys, (void*)n, tree_leaf_keys_max(tree) / 2);
	return keys;
}
static void leaf_num_keys_set(const struct clod_tree *tree, node *n, const unsigned short keys) {
	assert_fatal(CLOD_DEBUG, keys <= tree_leaf_keys_max(tree),
		"Key count %i out of bounds for tree with only %i keys per leaf.",
		keys, tree_leaf_keys_max(tree));
	assert_fatal(CLOD_DEBUG, !node_parent(tree, n) || keys >= tree_leaf_keys_max(tree) / 2,
		"Too few keys %i in node %ptr for tree with minimum %i keys per leaf node.",
		keys, (void*)n, tree_leaf_keys_max(tree) / 2);
	n->header[6] = (unsigned char)(keys >> 8);
	n->header[7] = (unsigned char)keys;
}
static node *leaf_next(const struct clod_tree *tree, const node *n) { return ptr(tree, (unsigned char*)n + 16); }
static void leaf_next_set(const struct clod_tree *tree, node *n, const node *next) { ptr_set(tree, (unsigned char*)n + 16, next); }
static node *leaf_prev(const struct clod_tree *tree, const node *n) { return ptr(tree, (unsigned char*)n + 24); }
static void leaf_prev_set(const struct clod_tree *tree, node *n, const node *prev) { ptr_set(tree, (unsigned char*)n + 24, prev); }
static char *leaf_key(const struct clod_tree *tree, const node *n, const unsigned short index) {
	assert_fatal(CLOD_DEBUG, index < tree_leaf_keys_max(tree),
		"Key index %i out of bounds for tree with only %i key/value pairs per leaf node.",
		index, tree_leaf_keys_max(tree));

	return (char*)n + HEADER_SIZE + index * tree->key_size;
}
static char *leaf_keys_end(const struct clod_tree *tree, const node *n) {
	return (char*)n + HEADER_SIZE + tree_leaf_keys_max(tree) * tree->key_size;
}
static char *leaf_val(const struct clod_tree *tree, const node *n, const unsigned short index) {
	assert_fatal(CLOD_DEBUG, index < tree_leaf_keys_max(tree),
		"Value index %i out of bounds for tree with only %i key/value pairs per leaf node.",
		index, tree_leaf_keys_max(tree));

	return (char*)n + HEADER_SIZE + tree_leaf_keys_max(tree) * tree->key_size + index * tree->val_size;
}
static char *leaf_vals_end(const struct clod_tree *tree, const node *n) {
	return (char*)n + HEADER_SIZE + tree_leaf_keys_max(tree) * (tree->key_size + tree->val_size);
}
static char *leaf_split(const struct clod_tree *tree, node *n, node *new) {
	unsigned short keys = leaf_num_keys(tree, n);
	assert_fatal(CLOD_DEBUG, keys > 1, "Cannot split node with zero keys.");

	unsigned short promoted_index = (keys + 1) / 2;
	unsigned short left_num_keys = promoted_index;
	unsigned short right_num_keys = keys - promoted_index;

	node_type_set(new, TYPE_LEAF);
	leaf_num_keys_set(tree, new, right_num_keys);
	node_parent_set(tree, new, node_parent(tree, n));
	leaf_next_set(tree, new, leaf_next(tree, n));
	leaf_prev_set(tree, new, n);

	leaf_prev_set(tree, leaf_next(tree, n), new);

	leaf_num_keys_set(tree, n, left_num_keys);
	leaf_next_set(tree, n, new);

	char *src = leaf_key(tree, n, left_num_keys);
	char *dst = leaf_key(tree, new, 0);
	for (size_t i = 0; i < right_num_keys * tree->key_size; i++) {
		dst[i] = src[i];
	}

	src = leaf_val(tree, n, left_num_keys);
	dst = leaf_val(tree, new, 0);
	for (size_t i = 0; i < right_num_keys * tree->val_size; i++) {
		dst[i] = src[i];
	}

	return leaf_key(tree, new, 0);
}
static void leaf_merge(const struct clod_tree *tree, node *n, const node *old_right) {
	unsigned short left_num_keys = leaf_num_keys(tree, n);
	unsigned short right_num_keys = leaf_num_keys(tree, old_right);

	leaf_num_keys_set(tree, n, left_num_keys + right_num_keys);
	leaf_next_set(tree, n, leaf_next(tree, old_right));
	leaf_prev_set(tree, leaf_next(tree, old_right), n);

	const char *src = leaf_key(tree, old_right, 0);
	char *dst = leaf_key(tree, n, left_num_keys);
	for (size_t i = 0; i < right_num_keys * tree->key_size; i++) {
		dst[i] = src[i];
	}

	src = leaf_val(tree, old_right, 0);
	dst = leaf_val(tree, n, left_num_keys);
	for (size_t i = 0; i < right_num_keys * tree->val_size; i++) {
		dst[i] = src[i];
	}
}
static bool leaf_insert(const struct clod_tree *tree, node *n, const unsigned short index, const char *key, const char *val) {
	unsigned short keys = leaf_num_keys(tree, n);
	if (keys == tree_leaf_keys_max(tree)) {
		return false;
	}

	leaf_num_keys_set(tree, n, keys + 1);

	const char *src = leaf_key(tree, n, index);
	char *dst = leaf_key(tree, n, index + 1);
	for (ptrdiff_t i = (keys - index) * tree->key_size; i >= index * tree->key_size; i--) {
		dst[i] = src[i];
	}
	src = key;
	dst = leaf_key(tree, n, index);
	for (size_t i = 0; i < tree->key_size; i++) {
		dst[i] = src[i];
	}

	src = leaf_val(tree, n, index);
	dst = leaf_val(tree, n, index + 1);
	for (ptrdiff_t i = (keys - index) * tree->val_size; i >= index * tree->val_size; i--) {
		dst[i] = src[i];
	}
	src = val;
	dst = leaf_val(tree, n, index);
	for (size_t i = 0; i < tree->val_size; i++) {
		dst[i] = src[i];
	}

	return true;
}
static void leaf_remove(const struct clod_tree *tree, node *n, const unsigned short index) {
	unsigned short keys = leaf_num_keys(tree, n);
	assert_fatal(CLOD_DEBUG, index < keys, "Key index %i out of bounds for node with only %i key/value pairs.", index, keys);

	leaf_num_keys_set(tree, n, keys - 1);

	const char *src = leaf_key(tree, n, index + 1);
	char *dst = leaf_key(tree, n, index);
	for (size_t i = 0; i < (keys - index - 1) * tree->key_size; i++) {
		dst[i] = src[i];
	}
	src = leaf_val(tree, n, index + 1);
	dst = leaf_val(tree, n, index);
	for (size_t i = 0; i < (keys - index - 1) * tree->val_size; i++) {
		dst[i] = src[i];
	}
}
static bool leaf_find(const struct clod_tree *tree, const node *n, const void *key, unsigned short *index) {
	unsigned short i = 0, l = 0, h = leaf_num_keys(tree, n);
	while (l < h) {
		i = (l + h) / 2;
		int cmp = tree->compare(leaf_key(tree, n, i), key);
		if (cmp < 0) {
			l = i + 1;
		} else if (cmp > 0) {
			h = i;
		} else {
			*index = i;
			return true;
		}
	}
	return false;
}

static unsigned short intl_num_keys(const struct clod_tree *tree, const node *n) {
	unsigned short keys = (unsigned short)(n->header[6] << 8) | n->header[7];
	assert_fatal(CLOD_MEMORY_CHECK, keys <= tree_intl_keys_max(tree),
		"Invalid key count %i in node %ptr for tree with only %i keys per internal node.",
		keys, (void*)n, tree_intl_keys_max(tree));
	assert_fatal(CLOD_DEBUG, !node_parent(tree, n) || keys >= tree_intl_keys_max(tree) / 2,
		"Too few keys %i in node %ptr for tree with minimum %i keys per internal node.",
		keys, (void*)n, tree_intl_keys_max(tree) / 2);
	return keys;
}
static void intl_num_keys_set(const struct clod_tree *tree, node *n, const unsigned short keys) {
	assert_fatal(CLOD_DEBUG, keys <= tree_intl_keys_max(tree),
		"Key count %i out of bounds for tree with only %i keys per internal node.",
		keys, tree_intl_keys_max(tree));
	assert_fatal(CLOD_DEBUG, !node_parent(tree, n) || keys >= tree_intl_keys_max(tree) / 2,
		"Too few keys %i in node %ptr for tree with minimum %i keys per internal node.",
		keys, (void*)n, tree_intl_keys_max(tree) / 2);
	n->header[6] = (unsigned char)(keys >> 8);
	n->header[7] = (unsigned char)keys;
}
static char *intl_key(const struct clod_tree *tree, const node *n, const unsigned short index) {
	assert_fatal(CLOD_DEBUG, index < tree_intl_keys_max(tree),
		"Key index %i out of bounds for tree with only %i keys per internal node.",
		index, tree_intl_keys_max(tree));

	return (char*)n + HEADER_SIZE + index * tree->key_size;
}
static char *intl_keys_end(const struct clod_tree *tree, const node *n) {
	return (char*)n + HEADER_SIZE + tree_intl_keys_max(tree) * tree->key_size;
}
static node *intl_branch(const struct clod_tree *tree, const node *n, const unsigned short index) {
	assert_fatal(CLOD_DEBUG, index < tree_intl_keys_max(tree) + 1,
		"Branch index %i out of bounds for tree with only %i branches per node.",
		index, tree_intl_keys_max(tree) + 1);

	return ptr(tree, (unsigned char*)n + HEADER_SIZE + tree_intl_keys_max(tree) * tree->key_size + index * 8);
}
static void intl_branch_set(const struct clod_tree *tree, node *n, const unsigned short index, const node *branch) {
	assert_fatal(CLOD_DEBUG, index < tree_intl_keys_max(tree) + 1,
		"Branch index %i out of bounds for tree with only %i branches per node.",
		index, tree_intl_keys_max(tree) + 1);

	ptr_set(tree, (unsigned char*)n + HEADER_SIZE + tree_intl_keys_max(tree) * tree->key_size + index * 8, branch);
}
static unsigned short intl_branch_index_get(const struct clod_tree *tree, const node *n, const node *branch) {
	for (unsigned short i = 0; i < intl_num_keys(tree, n) + 1; i++) {
		if (intl_branch(tree, n, i) == branch) {
			return i;
		}
	}
	fatal(CLOD_MEMORY_CHECK, "Branch %ptr not found in node %ptr.", (void*)branch, (void*)n);
	return 0;
}
static char *intl_split(const struct clod_tree *tree, node *n, node *new) {
	unsigned short keys = intl_num_keys(tree, n);
	assert_fatal(CLOD_DEBUG, keys > 1, "Cannot split node with zero keys.");

	unsigned short promoted_index = keys / 2;
	unsigned short left_num_keys = promoted_index;
	unsigned short right_num_keys = keys - promoted_index - 1;

	node_type_set(new, TYPE_INTL);
	intl_num_keys_set(tree, new, right_num_keys);
	node_parent_set(tree, new, node_parent(tree, n));

	intl_num_keys_set(tree, n, left_num_keys);

	char *src = intl_key(tree, n, left_num_keys);
	char *dst = intl_key(tree, new, 0);
	for (size_t i = 0; i < right_num_keys * tree->key_size; i++) {
		dst[i] = src[i];
	}

	for (unsigned short i = 0; i <= right_num_keys; i++) {
		node *branch = intl_branch(tree, n, i + left_num_keys + 1);
		node_parent_set(tree, branch, new);
		intl_branch_set(tree, new, i, branch);
	}

	return intl_key(tree, new, 0);
}
static void intl_merge(const struct clod_tree *tree, node *n, const char *demoted, const node *old_right) {
	unsigned short left_num_keys = intl_num_keys(tree, n);
	unsigned short right_num_keys = intl_num_keys(tree, old_right);

	intl_num_keys_set(tree, n, left_num_keys + right_num_keys);

	const char *src = demoted;
	char *dst = intl_key(tree, n, left_num_keys);
	for (size_t i = 0; i < tree->key_size; i++) {
		dst[i] = src[i];
	}

	src = intl_key(tree, old_right, 0);
	dst = intl_key(tree, n, left_num_keys + 1);
	for (size_t i = 0; i < right_num_keys * tree->key_size; i++) {
		dst[i] = src[i];
	}

	for (unsigned short i = 0; i <= right_num_keys; i++) {
		node *branch = intl_branch(tree, old_right, i);
		node_parent_set(tree, branch, n);
		intl_branch_set(tree, n, left_num_keys + 1 + i, branch);
	}
}
static bool intl_insert(const struct clod_tree *tree, node *n, const unsigned short index, const char *key, node *branch) {
	unsigned short keys = intl_num_keys(tree, n);
	if (keys == tree_intl_keys_max(tree)) {
		return false;
	}

	intl_num_keys_set(tree, n, keys + 1);

	const char *src = leaf_key(tree, n, index);
	char *dst = leaf_key(tree, n, index + 1);
	for (ptrdiff_t i = (keys - index) * tree->key_size; i >= index * tree->key_size; i--) {
		dst[i] = src[i];
	}
	src = key;
	dst = leaf_key(tree, n, index);
	for (size_t i = 0; i < tree->key_size; i++) {
		dst[i] = src[i];
	}

	for (unsigned short i = keys; i > index; i--) {
		intl_branch_set(tree, n, i + 1, intl_branch(tree, n, i));
	}
	intl_branch_set(tree, n, index + 1, intl_branch(tree, n, index));

	node_parent_set(tree, branch, n);
	intl_branch_set(tree, n, index, branch);
	return true;
}
static void intl_remove(const struct clod_tree *tree, node *n, const unsigned short index) {
	unsigned short keys = intl_num_keys(tree, n);
	assert_fatal(CLOD_DEBUG, index < keys, "Key index %i out of bounds for node with only %i key/value pairs.", index, keys);

	intl_num_keys_set(tree, n, keys - 1);
	const char *src = intl_key(tree, n, index + 1);
	char *dst = intl_key(tree, n, index);
	for (size_t i = 0; i < (keys - index - 1) * tree->key_size; i++) {
		dst[i] = src[i];
	}
	for (unsigned short i = index; i <= keys; i++) {
		intl_branch_set(tree, n, i, intl_branch(tree, n, i + 1));
	}
}
static bool intl_find(const struct clod_tree *tree, const node *n, const void *key, unsigned short *index) {
	unsigned short i = 0, l = 0, h = intl_num_keys(tree, n);
	while (l < h) {
		i = (l + h) / 2;
		int cmp = tree->compare(intl_key(tree, n, i), key);
		if (cmp < 0) {
			l = i + 1;
		} else if (cmp > 0) {
			h = i;
		} else {
			*index = i;
			return true;
		}
	}
	return false;
}

static bool root_split(const struct clod_tree *tree, node *n) {
	node *new_left = tree->alloc(tree->alloc_user, tree->node_size);
	if (!new_left) {
		debug(CLOD_DEBUG, "Failed to allocate new node.");
		return false;
	}

	node *new_right = tree->alloc(tree->alloc_user, tree->node_size);
	if (!new_right) {
		debug(CLOD_DEBUG, "Failed to allocate new node.");
		tree->free(tree->alloc_user, new_left);
		return false;
	}

	for (size_t i = 0; i < tree->node_size; i++) {
		((char*)new_left)[i] = ((char*)n)[i];
	}

	char *promoted_key;
	if (node_type(n) == TYPE_LEAF) {
		promoted_key = leaf_split(tree, new_left, new_right);
		leaf_prev_set(tree, new_left, nullptr);
		leaf_next_set(tree, new_left, new_right);
		leaf_prev_set(tree, new_right, new_left);
		leaf_next_set(tree, new_right, nullptr);
	} else {
		promoted_key = intl_split(tree, new_left, new_right);
	}

	node_parent_set(tree, new_left, n);
	node_parent_set(tree, new_right, n);

	node_type_set(n, TYPE_INTL);
	intl_num_keys_set(tree, n, 1);
	intl_branch_set(tree, n, 0, new_left);
	intl_branch_set(tree, n, 1, new_right);

	char *dst = intl_key(tree, n, 0);
	for (size_t i = 0; i < tree->key_size; i++) {
		dst[i] = promoted_key[i];
	}

	return true;
}
static void root_merge(const struct clod_tree *tree, node *n) {
	assert_fatal(CLOD_DEBUG, node_type(n) == TYPE_INTL, "Cannot merge non-internal root node %ptr.", (void*)n);
	assert_fatal(CLOD_DEBUG, intl_num_keys(tree, n) == 1, "Cannot merge root node with %i keys.", intl_num_keys(tree, n));

	node *left = intl_branch(tree, n, 0), *right = intl_branch(tree, n, 1);
	if (node_type(left) == TYPE_LEAF) {
		leaf_merge(tree, left, right);
	} else {
		intl_merge(tree, left, intl_key(tree, left, 0), right);
		for (unsigned short i = 0; i <= intl_num_keys(tree, left); i++) {
			node_parent_set(tree, intl_branch(tree, left, i), left);
		}
	}

	for (size_t i = 0; i < tree->node_size; i++) {
		((char*)n)[i] = ((char*)left)[i];
	}

	tree->free(tree->alloc_user, left);
	tree->free(tree->alloc_user, right);
}

static bool tree_split(const struct clod_tree *tree, node *n) {
	if (!node_parent(tree, n)) {
		return root_split(tree, n);
	}

	if (intl_num_keys(tree, node_parent(tree, n)) >= tree_intl_keys_max(tree)) {
		if (!tree_split(tree, node_parent(tree, n))) {
			return false;
		}
	}

	node *new = tree->alloc(tree->alloc_user, tree->node_size);
	if (!new) {
		debug(CLOD_DEBUG, "Failed to allocate new node.");
		return false;
	}

	char *promoted = (node_type(n) == TYPE_LEAF) ? leaf_split(tree, n, new) : intl_split(tree, n, new);
	node *parent = node_parent(tree, n);
	if (!intl_insert(tree, parent, intl_branch_index_get(tree, parent, n), promoted, new)) {
		fatal(CLOD_MEMORY_CHECK, "Failed to insert new node %ptr into parent %ptr.", new, parent);
	}
	return true;
}
static char *tree_parent_key(const struct clod_tree *tree, const node *n) {
	while (node_parent(tree, n)) {
		unsigned short index = intl_branch_index_get(tree, node_parent(tree, n), n);
		n = node_parent(tree, n);

		if (index > 0) {
			return intl_key(tree, n, index - 1);
		}
	}
	return nullptr;
}
static void tree_balance_intl(const struct clod_tree *tree, node *intl) {
	if (leaf_num_keys(tree, intl) > tree_intl_keys_max(tree) / 2) {
		return;
	}


}
static void tree_balance_leaf(const struct clod_tree *tree, node *leaf) {
	if (leaf_num_keys(tree, leaf) > tree_leaf_keys_max(tree) / 2) {
		return;
	}

	node *left, *right;
	if (leaf_next(tree, leaf) && node_parent(tree, leaf_next(tree, leaf)) == node_parent(tree, leaf)) {
		left = leaf;
		right = leaf_next(tree, leaf);
	} else {
		left = leaf_prev(tree, leaf);
		right = leaf;
	}

	if (leaf_num_keys(tree, left) + leaf_num_keys(tree, right) <= tree_leaf_keys_max(tree)) {
		unsigned short parent_index = intl_branch_index_get(tree, node_parent(tree, leaf), leaf);
		intl_remove()
	}

	char *parent_key = tree_parent_key(tree, right);

	node *left = leaf, *right = leaf_next(tree, leaf);
	char *parent_key = tree_parent_key(tree, leaf);
	if (!right) {
		right = leaf;
		left = leaf_prev(tree, leaf);
	}

	if (leaf_num_keys(tree, left) + leaf_num_keys(tree, right) <= tree_leaf_keys_max(tree)) {
		intl_remove(tree, node_parent(tree, right), intl_branch_index_get(tree, node_parent(tree, right), right));
		leaf_merge(tree, left, right);
		tree_balance_intl(tree, node_parent(tree, left));
	}
}
static void tree_remove(const struct clod_tree *tree, node *n, const unsigned short index) {
	assert_fatal(CLOD_DEBUG, node_type(n) == TYPE_LEAF, "Cannot remove from non-leaf node %ptr.", (void*)n);

	if (index == 0) {
		const char *src = leaf_key(tree, n, 1);
		char *dst = tree_parent_key(tree, n);
		if (dst) for (size_t i = 0; i < tree->key_size; i++) {
			dst[i] = src[i];
		}
	}

	leaf_remove(tree, n, index);

	if (node_parent(tree, n) && leaf_num_keys(tree, n) <= tree_leaf_keys_max(tree) / 2) {
		tree_balance_leaf(tree, n);
	}

	return;

	if (node_parent(tree, n) && leaf_num_keys(tree, n) <= tree_leaf_keys_max(tree) / 2) {
		node *next = leaf_next(tree, n);
		const char *parent_key = tree_parent_key(tree, next);


		if (leaf_num_keys(tree, next) > tree_leaf_keys_max(tree) / 2) {
			char *key = leaf_key(tree, next, 0);
			leaf_insert(tree, n, leaf_num_keys(tree, n), key, leaf_val(tree, next, 0));

			for (node *left = n, *right = next; node_parent(tree, left);) {
				unsigned short intl_index = intl_branch_index_get(tree, node_parent(tree, left), left);
				left = node_parent(tree, left);
				right = node_parent(tree, right);

				if (left == right) {

				}
			}
		}
	}
}
static void tree_destroy(const struct clod_tree *tree, node *intl) {
	for (unsigned short i = 0; i < intl_num_keys(tree, intl); i++) {
		node *child = intl_branch(tree, intl, i);
		if (node_type(child) == TYPE_INTL) {
			tree_destroy(tree, child);
		}
		tree->free(tree->alloc_user, child);
	}
}

bool clod_tree_create(struct clod_tree *tree) {
	if (tree->key_size == 0) {
		debug(CLOD_DEBUG, "Tree key size cannot be zero.");
		return false;
	}
	if (tree->node_size < HEADER_SIZE + tree->key_size + (tree->val_size < 16 ? 16 : tree->val_size)) {
		debug(CLOD_DEBUG, "Tree node size of %i is too small. Need %i bytes for key_size of %i and val_size of %i.",
			tree->node_size, HEADER_SIZE + tree->key_size + (tree->val_size < 16 ? 16 : tree->val_size));
		return false;
	}
	if (!tree->compare) {
		debug(CLOD_DEBUG, "Tree compare function cannot be null.");
		return false;
	}

	if (!tree->root) {
		tree->root = tree->alloc(tree->alloc_user, tree->node_size);
		if (!tree->root) {
			debug(CLOD_DEBUG, "Failed to allocate root node.");
			return false;
		}
	}

	return true;
}
void clod_tree_destroy(struct clod_tree *tree) {
	tree_destroy(tree, tree->root);
	tree->free(tree->alloc_user, tree->root);
	tree->root = nullptr;
}
void *clod_tree_get(const struct clod_tree *tree, const void *key) {
	node *n = tree->root;
	unsigned short index;
	while (node_type(n) == TYPE_INTL) {
		intl_find(tree, n, key, &index);
		n = intl_branch(tree, n, index);
	}

	if (leaf_find(tree, n, key, &index)) {
		return leaf_val(tree, n, index);
	}

	return nullptr;
}
void *clod_tree_add(const struct clod_tree *tree, const void *key, const void *val) {
	node *n = tree->root;
	unsigned short index;
	while (node_type(n) == TYPE_INTL) {
		intl_find(tree, n, key, &index);
		n = intl_branch(tree, n, index);
	}

	if (leaf_find(tree, n, key, &index)) {
		return leaf_val(tree, n, index);
	}

	if (leaf_insert(tree, n, index, key, val)) {
		return nullptr;
	}

	if (!split(tree, n)) {
		debug(CLOD_DEBUG, "Failed to split node %ptr.", n);
		return (void*)key;
	}

	if (!leaf_insert(tree, n, index, key, val)) {
		fatal(CLOD_MEMORY_CHECK, "Failed to insert key %ptr into node %ptr.", (void*)key, (void*)n);
	}

	return nullptr;
}
void *clod_tree_del(const struct clod_tree *tree, const void *key) {
	node *n = tree->root;
	unsigned short index;
	while (node_type(n) == TYPE_INTL) {
		intl_find(tree, n, key, &index);
		n = intl_branch(tree, n, index);
	}

	if (leaf_find(tree, n, key, &index)) {

	}
}