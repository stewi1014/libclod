/**
 *@file clod/structures/tree.h
 *@defgroup tree Tree
 */
#ifndef LIBCLOD_TREE_H
#define LIBCLOD_TREE_H

#include <stdio.h>
#include <clod/lib.h>

#include "clod/memory.h"

enum clod_tree_result {
	CLOD_TREE_OK = 0,
	CLOD_TREE_ALREADY_EXISTS = 1,
	CLOD_TREE_NOT_FOUND = 2,
	CLOD_TREE_CORRUPTED = 3,
	CLOD_TREE_INVALID = 4,
	CLOD_TREE_ALLOC_FAILED = 5,

};

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
	unsigned char *root_node;

	/// Size of the valid memory behind \p root.
	/// Used to validate pointers to nodes within the tree.
	/// If 0, the checks are disabled.
	size_t size;

	/// Size of keys in the tree.
	unsigned char key_size;

	/// Size of values in the tree.
	unsigned char val_size;

	/// Capacity of internal nodes in keys.
	unsigned short intl_cap;

	/// Capacity of leaf nodes in keys.
	unsigned short leaf_cap;

	/// Enable shadowing of writes.
	bool shadow;

	/// Disables the checksum used to validate each node.
	bool disable_checksum;

	/// Seed value used to perform integrity checks of tree nodes.
	uint32_t checksum_seed;

	/// User variable passed to invocations of \p compare.
	void *compare_user;

	/// Comparison function.
	int (*compare)(const char *key1, const char *key2, void *user);

	/// Allocator used to allocate and free nodes in the tree.
	clod_allocator allocator;
};

/**
 * Set the tree's \p intl_cap and \p leaf_cap so that nodes
 * have a size equal to or smaller than \p node_size.
 *
 * Must be called before clod_tree_create.
 *
 * @param[in,out] tree Tree to configure.
 * @param[in] node_size Maximum node size.
 * @return True if successful, false if node_size is too small to honour.
 */
CLOD_API CLOD_NONNULL(1)
bool clod_tree_set_node_size(struct clod_tree *tree, unsigned short node_size);

/**
 * Initialise a tree.
 * @param[in] tree Tree to initialise.
 * @return True on success, false if parameters failed validation.
 */
CLOD_API CLOD_NONNULL(1)
enum clod_tree_result clod_tree_create(struct clod_tree *tree);

/**
 * Destroy a tree. All nodes are freed.
 * @param[in] tree Tree to destroy.
 */
CLOD_API CLOD_NONNULL(1)
void clod_tree_destroy(struct clod_tree *tree);

/**
 * Commit shadowed changes to the tree.
 * Called after synchronising tree data with its backing storage.
 * @param[in] tree Tree to commit changes to.
 */
CLOD_API CLOD_NONNULL(1)
void clod_tree_commit(struct clod_tree *tree);

/**
 * Commit shadowed changes to the tree.
 * Called after synchronising tree data with its backing storage.
 * @param[in] tree Tree to commit changes to.
 */
CLOD_API CLOD_NONNULL(1)
void clod_tree_rollback(struct clod_tree *tree);

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
enum clod_tree_result clod_tree_add(const struct clod_tree *tree, const char *key, const char *val, char *key_out, char *val_out);

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
enum clod_tree_result clod_tree_get(const struct clod_tree *tree, const char *key, char *key_out, char *val_out);

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
enum clod_tree_result clod_tree_del(const struct clod_tree *tree, const char *key, char *key_out, char *val_out);

/**
 * Represents a specific location within the tree, allowing adjacent elements to be
 * quickly found, or the same element to be interacted with multiple times, without
 * multiple redundant searches of the tree.
 */
struct clod_tree_location {
	unsigned char *node;
	int index;
	char *key;
	char *val;
};

/**
 * Find the element equal to, or the next smaller than, the provided key.
 *
 * @param[in] tree Tree to search in.
 * @param[out] location Location of the search result.
 * @param[in] key Pointer to a key of the configured size.
 * @return Zero if the key is found, otherwise non-zero error value.
 */
CLOD_API CLOD_NONNULL(1, 2, 3)
enum clod_tree_result clod_tree_find(const struct clod_tree *tree, struct clod_tree_location *location, const char *key);

/**
 * Get the next element in the tree.
 * @param[in] tree Tree to get next element in.
 * @param[in,out] location Location of the current element.
 * If the location is not initialised (null node member), the first element in the tree is returned.
 * @return Zero if the next element was found, otherwise non-zero error value.
 */
CLOD_API CLOD_NONNULL(1, 2)
enum clod_tree_result clod_tree_next(const struct clod_tree *tree, struct clod_tree_location *location);

/**
 * Get the previous element in the tree.
 * @param[in] tree Tree to get the previous element from.
 * @param[in,out] location Location of the current element.
 * If the location is not initialised (null node member), the last element in the tree is returned.
 * @return Zero if the previous element was found, otherwise non-zero error value.
 */
CLOD_API CLOD_NONNULL(1, 2)
enum clod_tree_result clod_tree_prev(const struct clod_tree *tree, struct clod_tree_location *location);

/**
 * Insert the key and value in \p location into the tree at the specific location.
 *
 * The caller is responsible for ensuring the inserted key is in order with respect to other elements in the tree.
 *
 * @param[in] tree Tree to insert the element into.
 * @param[in] location Location and element to insert.
 * @return Zero on success, otherwise non-zero error value.
 */
CLOD_API CLOD_NONNULL(1, 2)
enum clod_tree_result clod_tree_insert(const struct clod_tree *tree, struct clod_tree_location *location);

/**
 * Replace the key and/or value at the given location.
 * The caller is responsible for ensuring the replaced key is in order with respect to other elements in the tree.
 *
 * @param tree
 * @param location
 * @return
 */
CLOD_API CLOD_NONNULL(1, 2)
enum clod_tree_result clod_tree_replace(const struct clod_tree *tree, struct clod_tree_location *location);


/**
 * Check the entire tree for validity. It prints a debug message if enabled.
 * A completely empty (tree->root == nullptr) tree is considered valid.
 * @param[in] tree Tree to check for consistency.
 * @return True if the tree is valid, false if the tree is malformed.
 */
CLOD_API CLOD_NONNULL(1)
bool clod_tree_validate(const struct clod_tree *tree);

/**
 * Print the tree in human-readable form to the given stream.
 * Useful for debugging.
 * @param[in] tree Tree to print.
 * @param[out] dst Stream to print to.
 * @param[in] max_depth Maximum depth to print to.
 * @return Error code returned by \p dst->write
 */
CLOD_API CLOD_NONNULL(1, 2)
int clod_tree_print(const struct clod_tree *tree, clod_stream *dst, int max_depth);

#endif
