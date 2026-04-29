#include "node.h"

#define TABS\
	"|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t"\
	"|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t|\t"

static int clod_tree_print_recursive(
	const struct clod_btree *tree,
	clod_stream *dst,
	const node *n,
	const int depth,
	const int max_depth,
	struct clod_btree_location *highlight
) {
	int res;
	#define print(fmt, ...) {\
		res = clod_stream_format(dst, CLOD_STRING_C(fmt) __VA_OPT__(,) __VA_ARGS__);\
		if (res != 0) {\
			return res;\
		}\
	}

	if (depth >= max_depth) {
		print("%.*s...\n", TABS, depth);
	}

	if (n == nullptr) {
		print("%.*sNULL\n", TABS, depth);
	}

	if (tree->size && (n < tree->root_node || n > tree->root_node + tree->size)) {
		print("%.*sINVALID POINTER %ptr\n", TABS, depth, (void*)n);
	}

	if (depth_get(n) == 0) {
		if (n == tree->root_node) {
			print("%.*sTree %ptr (Leaf Root: %ptr, Keys: %u, Checksum: %Xu32)\n",
				TABS, depth, (void*)tree, (void*)n, length_get(n),
				checksum_get(n));
		} else {
			print(
				"%.*sLeaf %ptr (Parent: %ptr, Next: %ptr, Prev: %ptr, Keys: %u, Checksum: %Xu32)\n",
				TABS, depth, (void*)n, (void*)parent_get(n), (void*)leaf_next_get(n),
				(void*)leaf_prev_get(n), length_get(n), checksum_get(n)
			);
		}

		for (int i = 0; i < length_get(n); i++) {
			if (highlight && n == highlight->node && i == highlight->index) {
				if (
					(highlight->key != nullptr && highlight->key != leaf_key(n, i)) ||
					(highlight->val != nullptr && highlight->val != leaf_val(n, i))
				) {
					print(
						"=>\t%.*s [%.*mem] -> %.*mem => [%.*mem] -> %.*mem\n",
						TABS, depth,
						(void*)leaf_key(n, i), tree->key_size, (void*)leaf_val(n, i), tree->val_size,
						(void*)highlight->key, tree->key_size, (void*)highlight->val, tree->val_size
					);
				} else {
					print(
						"=>\t%.*s [%.*mem] -> %.*mem\n",
						TABS, depth,
						(void*)leaf_key(n, i), tree->key_size, (void*)leaf_val(n, i), tree->val_size
					);
				}
			} else {
				print(
					"  \t%.*s [%.*mem] -> %.*mem\n",
					TABS, depth,
					(void*)leaf_key(n, i), tree->key_size, (void*)leaf_val(n, i), tree->val_size
				);
			}
		}

	} else {
		if (n == tree->root_node) {
			print(
				"%.*sTree %ptr (Intl Root: %ptr, Keys: %u, Checksum: %Xu32)\n",
				TABS, depth, (void*)tree, (void*)n, length_get(n), checksum_get(n)
			);
		} else {
			print(
				"%.*sIntl %ptr (Parent: %ptr, Keys: %u, Checksum: %Xu32)\n",
				TABS, depth, (void*)n, (void*)parent_get(n), length_get(n), checksum_get(n)
			);
		}

		for (int i = 0; i < length_get(n); i++) {
			clod_tree_print_recursive(tree, dst, intl_branch_get(n, i), depth + 1, max_depth, highlight);
			print("|>%.*sKey: %.*mem\n", TABS, depth, (void*)intl_key(n, i), tree->key_size);
		}
		clod_tree_print_recursive(tree, dst, intl_branch_get(n, length_get(n)), depth + 1, max_depth, highlight);
	}

	return CLOD_ERR_OK;
}

int clod_tree_print(const struct clod_btree *tree, clod_stream *dst, const int max_depth) {
	return clod_tree_print_recursive(tree, dst, tree->root_node, 0, max_depth, nullptr);
}