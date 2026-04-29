#include "node.h"
#include <clod/hash.h>

/// Bytes at start of nore to skip checksum for.
#define CHECKSUM_OFFSET 12
#define CHECKSUM_FLAGS UINT32_C(1)
#define CHECKSUM_MODIFY_FLAG UINT32_C(1)

CLOD_PURE
uint32_t checksum_compute(const struct clod_btree *tree, const node *n) {
	switch (tree->check) {

		case CLOD_BTREE_CHECK_CANARY:
			return tree->check_seed &~ CHECKSUM_FLAGS;

		case CLOD_BTREE_CHECK_CRC32:

			uint32_t hash = tree->check_seed;
			const ptrdiff_t offset = n - tree->root_node;

			hash = clod_crc32_add(hash, n + CHECKSUM_OFFSET, tree->node_size);
			hash = clod_crc32_add(hash, &offset, sizeof(offset));

			return hash &~ CHECKSUM_FLAGS;

		default:
			return 0;
	}
}

struct validate_state {
	const void *last_key;
	const node *last_leaf;
	bool last_key_was_intl;
};

bool leaf_validate(
	const struct clod_btree *tree,
	const node *n,
	const node *parent,
	struct validate_state *state,
	const int depth
) {
	bool ok = true;

	if (state->last_leaf && leaf_next_get(state->last_leaf) != n) {
		debug(CLOD_DEBUG, "Leaf node %ptr has %ptr as the next leaf node, but the actual next leaf node is %ptr.",
			(void*)state->last_leaf, (void*)leaf_next_get(state->last_leaf), (void*)n);

		ok = false;
	}

	if (depth != depth_get(n) || depth_get(n) != 0) {
		debug(CLOD_DEBUG, "Leaf node %ptr is at incorrect depth %u for tree with depth %u.",
			(void*)n, depth_get(tree->root_node) - depth, depth_get(tree->root_node));

		ok = false;
	}

	if (leaf_prev_get(n) != state->last_leaf) {
		debug(CLOD_DEBUG, "Leaf node %ptr has %ptr as the previous leaf node, but the actual previous leaf node is %ptr.",
			(void*)n, (void*)leaf_prev_get(n), (void*)state->last_leaf);

		ok = false;
	}

	if (parent_get(n) != parent) {
		debug(CLOD_DEBUG, "Leaf node %ptr has %ptr as the parent node, but the actual parent node is %ptr.",
			(void*)n, (void*)parent_get(n), (void*)parent);

		ok = false;
	}

	if (parent_get(n) && length_get(n) < LEAF_CAPACITY / 2) {
		debug(CLOD_DEBUG, "Leaf node %ptr has %u keys which is less than half the capacity (%u out of %u total) required for a balanced tree.",
			(void*)n, length_get(n), LEAF_CAPACITY / 2, LEAF_CAPACITY);

		ok = false;
	}

	if (length_get(n) > LEAF_CAPACITY) {
		debug(CLOD_DEBUG, "Leaf node %ptr has %u keys, which is more than the capacity (%u).",
			(void*)n, length_get(n), LEAF_CAPACITY);

		ok = false;
	}

	for (int i = 0; i < length_get(n); i++) {
		if (state->last_key) {
			const int cmp = tree->compare(leaf_key(n, i), state->last_key, tree->compare_user);
			if (state->last_key_was_intl ? cmp < 0 : cmp <= 0) {
				debug(CLOD_DEBUG, "Leaf node %ptr has key [%.*mem] at index index %u which is out of order with previous key [%.*mem].",
					(void*)n, (void*)leaf_key(n, i), tree->key_size, i, (void*)state->last_key, tree->key_size);

				ok = false;
			}
		}

		state->last_key = leaf_key(n, i);
		state->last_leaf = n;
		state->last_key_was_intl = false;
	}

	return ok;
}

bool intl_validate(
	const struct clod_btree *tree,
	const node *n,
	const node *parent,
	struct validate_state *state,
	const int depth
) {
	bool ok = true;

	if (depth != depth_get(n) || depth_get(n) <= 0) {
		debug(CLOD_DEBUG, "Internal node %ptr is at incorrect depth %u for tree with depth %u.",
			(void*)n, depth_get(tree->root_node) - depth, depth_get(tree->root_node));

		ok = false;
	}

	if (parent_get(n) != parent) {
		debug(CLOD_DEBUG, "Internal node %ptr has %ptr as the parent node, but the actual parent node is %ptr.",
			(void*)n, (void*)parent_get(n), (void*)parent);

		ok = false;
	}

	if (parent_get(n) && length_get(n) < INTL_CAPACITY / 2) {
		debug(CLOD_DEBUG, "Internal node %ptr has %u keys which is less than half the capacity (%u out of %u total) required for a balanced tree.",
			(void*)n, length_get(n), INTL_CAPACITY / 2, INTL_CAPACITY);

		ok = false;
	}

	if (length_get(n) > INTL_CAPACITY) {
		debug(CLOD_DEBUG, "Internal node %ptr has %u keys, which is more than the capacity (%u).",
			(void*)n, length_get(n), INTL_CAPACITY);

		ok = false;
	}

	for (int i = 0; i < length_get(n); i++) {
		const node *branch = intl_branch_get(n, i);
		if (!branch) {
			debug(CLOD_DEBUG, "Internal node %ptr has null branch at index %u.",
				(void*)n, i);

			ok = false;
		}

		if (state->last_key) {
			if (tree->compare(intl_key(n, i), state->last_key, tree->compare_user) <= 0) {
				debug(CLOD_DEBUG, "Internal node %ptr has key [%.*mem] at index index %u which is out of order with previous key [%.*mem].",
					(void*)n, (void*)intl_key(n, i), tree->key_size, i, (void*)state->last_key, tree->key_size);

				ok = false;
			}
		}

		state->last_key = intl_key(n, i);
		state->last_key_was_intl = true;
	}

	const node *branch = intl_branch_get(n, length_get(n));
	if (!branch) {
		debug(CLOD_DEBUG, "Internal node %ptr has null branch at index %u.",
			(void*)n, length_get(n));

		ok = false;
	}

	return ok;
}

bool intl_validate_recursive(
	const struct clod_btree *tree,
	const node *n,
	const node *parent,
	struct validate_state *state,
	const int depth
) {
	bool ok = intl_validate(tree, n, parent, state, depth);

	for (int i = 0; i <= length_get(n); i++) {
		const node *branch = intl_branch_get(n, i);
		if (depth_get(branch) == 0) {
			ok &= leaf_validate(tree, branch, n, state, depth - 1);
		} else {
			ok &= intl_validate_recursive(tree, branch, n, state, depth - 1);
		}
	}

	return ok;
}

enum clod_btree_result clod_tree_validate(const struct clod_btree *tree) {
	if (tree->root_node == nullptr) {
		return CLOD_BTREE_OK;
	}

	struct validate_state state = {0};

	bool ok = true;

	if (depth_get(tree->root_node) > 0) {
		ok &= intl_validate(tree, tree->root_node, nullptr, &state, depth_get(tree->root_node));
	} else {
		ok &= leaf_validate(tree, tree->root_node, nullptr, &state, depth_get(tree->root_node));
	}

	#if CLOD_DEBUG
	if (!ok) {
		clod_tree_print(tree, clod_stream_stderr, 5);
	}
	#endif

	if (ok) {
		return CLOD_BTREE_OK;
	}
	return CLOD_BTREE_CORRUPTED;
}
