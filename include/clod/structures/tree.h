/**
 *@file clod/structures/tree.h
 *@defgroup tree Tree
 */
#ifndef LIBCLOD_TREE_H
#define LIBCLOD_TREE_H

#include <clod/lib.h>

#include "clod/memory.h"

/**
 * Configuration options and handle to a tree structure.
 * Must not be modified after creation.
 *
 * Keys and values are aligned to the maximum of 32 or the largest factor of their sizes,
 * whichever is smaller. E.g. a size of 8 results in an alignment of 8, while 1, 3, or 31
 * result in no alignment. In addition, values inherit the key's alignment.
 *
 * The caller always retains ownership of memory passed as keys and values,
 * as the tree implementation always copies to its own memory.
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
	unsigned short node_size;

	/// User variable passed to invocations of \p compare.
	void *compare_user;

	/// Comparison function.
	int (*compare)(const void *key1, const void *key2, void *user);

	/// Allocator used to allocate and free nodes in the tree.
	///
	clod_allocator allocator;
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
 * If the key compares equal to an existing key, the operation fails,
 * optionally returning the existing key and value.
 *
 * @param[in] tree Tree to add the key/value pair to.
 * @param[in] key Pointer to a key of the configured size.
 * @param[in] val Pointer to a value of the configured size.
 * @param[out] key_out (nullable) Where the existing key is written to if the key already exists.
 * @param[out] val_out (nullable) Where the existing value is written to if the key already exists.
 * @return True on success. False otherwise.
 */
CLOD_API CLOD_NONNULL(1, 2, 3)
bool clod_tree_add(const struct clod_tree *tree, const void *key, const void *val, void *key_out, void *val_out);

/**
 * Get a key/value pair from the tree.
 *
 * @param[in] tree Tree to get the key/value from.
 * @param[in] key Pointer to a key of the configured size.
 * @param[out] key_out (nullable) Where the existing key is written to if the key already exists.
 * @param[out] val_out (nullable) Where the existing value is written to if the key already exists.
 * @return True if the given key was found. False otherwise.
 */
CLOD_API CLOD_NONNULL(1, 2)
bool clod_tree_get(const struct clod_tree *tree, const void *key, void *key_out, void *val_out);

/**
 * Remove a key/value pair from the tree.
 *
 * @param[in] tree Tree to remove the key/value pair from.
 * @param[in] key Pointer to a key of the configured size.
 * @param[out] key_out (nullable) Where the existing key is written to if the key already exists.
 * @param[out] val_out (nullable) Where the existing value is written to if the key already exists.
 * @return True if the given key was successfully found and removed. False otherwise.
 */
CLOD_API CLOD_NONNULL(1, 2)
bool clod_tree_del(const struct clod_tree *tree, const void *key, void *key_out, void *val_out);

/**
 * Represents a specific location within the tree, allowing adjacent elements to be
 * quickly found, or the same element to be interacted with multiple times, without
 * multiple redundant searches of the tree.
 */
struct clod_tree_location {
	struct clod_tree_node *node;
	const void *key;
	void *val;
};

/**
 * Find the element equal to, or the next lower element from, the provided key.
 * If an exact match is found, the method returns true.
 *
 * @param[in] tree Tree to search in.
 * @param[out] location Location of the search result.
 * @param[in] key Pointer to a key of the configured size.
 * @return True if an exact match was found. False otherwise.
 */
CLOD_API CLOD_NONNULL(1, 2, 3)
bool clod_tree_find(struct clod_tree *tree, struct clod_tree_location *location, const void *key);

/**
 * Get the next element in the tree.
 * @param[in] tree Tree to get next element in.
 * @param[in,out] location Location of the current element.
 * If the location is not initialised (null node member), the first element in the tree is returned.
 * @return True if the next element existed. False otherwise.
 */
CLOD_API CLOD_NONNULL(1, 2)
bool clod_tree_next(struct clod_tree *tree, struct clod_tree_location *location);

/**
 * Get the previous element in the tree.
 * @param[in] tree Tree to get the previous element from.
 * @param[in,out] location Location of the current element.
 * If the location is not initialised (null node member), the last element in the tree is returned.
 * @return True if the next element existed. False otherwise.
 */
CLOD_API CLOD_NONNULL(1, 2)
bool clod_tree_prev(struct clod_tree *tree, struct clod_tree_location *location);

/**
 * Check the entire tree for validity. It prints a debug message if enabled.
 * A completely empty (tree->root == nullptr) tree is considered valid.
 * @param[in] tree Tree to check for consistency.
 * @return True if the tree is valid, false if the tree is malformed.
 */
CLOD_API CLOD_NONNULL(1)
bool clod_tree_validate(const struct clod_tree *tree);

#endif
