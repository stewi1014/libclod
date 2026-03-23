#ifndef LIBCLOD_TREE_H
#define LIBCLOD_TREE_H

#include <clod/lib.h>

/**
 * Configuration options and handle to a tree structure.
 * Must not be modified after creation.
 *
 * Keys and values are aligned to the maximum of 32 or the largest factor of their sizes,
 * whichever is smaller. E.g. a size of 8 results in an alignment of 8, while 1, 3, or 31
 * result in no alignment. In addition, values inherit the key's alignment.
 */
struct clod_tree {
	/// Pointer to the root node.
	struct clod_tree_node *root;

	/// Size of keys in the tree.
	unsigned char key_size;

	/// Size of values in the tree.
	unsigned char val_size;

	/// Disables the checksum used to validate each node.
	bool disable_checksum;

	/// Size of each node in the tree.
	/// Calls to \p alloc are always a multiple of this.
	unsigned short node_size;

	/// Comparison function.
	int (*compare)(const void *key1, const void *key2);

	/// Passed to invocations of \p alloc and \p free.
	void *alloc_user;

	/// Method used to allocate memory.
	/// \p size is always a multiple of sizeof(struct clod_tree).
	void *(*alloc)(void *alloc_user, size_t size);

	/// Method used to free memory.
	void (*free)(void *alloc_user, void *ptr);
};

/**
 * Initialise a tree.
 * @param[in] tree Tree to initialise.
 * @return True on success, false if parameters failed validation.
 */
CLOD_API CLOD_NONNULL(1)
bool clod_tree_create(struct clod_tree *tree);

/**
 * Destroy a tree. All nodes are freed.
 * @param[in] tree Tree to destroy.
 */
CLOD_API CLOD_NONNULL(1)
void clod_tree_destroy(struct clod_tree *tree);

/**
 * Add a key/value pair to the tree.
 * @param tree
 * @param key
 * @param val
 * @return
 */
void *clod_tree_add(const struct clod_tree *tree, const void *key, const void *val);
void *clod_tree_get(const struct clod_tree *tree, const void *key);
void *clod_tree_del(const struct clod_tree *tree, const void *key);

struct clod_tree_location {
	struct clod_tree_node *node;
	void *key;
	void *val;
};

struct clod_tree_location clod_tree_find(struct clod_tree *tree, const void *key);
bool clod_tree_next(struct clod_tree *tree, struct clod_tree_location *iter);
bool clod_tree_prev(struct clod_tree *tree, struct clod_tree_location *iter);


#endif
