#ifndef LIBCLOD_TREE_H
#define LIBCLOD_TREE_H

#include <clod/lib.h>
#include <clod/memory.h>
#include <clod/stream/stream.h>

typedef unsigned char node;

enum clod_btree_check {
	/// No consistency checking is performed.
	CLOD_BTREE_CHECK_NONE = 0,

	/// A canary value at the start of every node (check_seed) is checked
	/// to confirm it hasn't been overwritten.
	CLOD_BTREE_CHECK_CANARY = 1,

	/// Each node is check summed before reading to ensure consistency.
	CLOD_BTREE_CHECK_CRC32 = 2
};

enum clod_btree_result {
	/// No worries.
	CLOD_BTREE_OK = 0,

	/// The element already exists.
	CLOD_BTREE_EXISTS = 1,

	/// The element does not exist.
	CLOD_BTREE_NONEXIST = 2,

	/// The tree appears corrupted.
	CLOD_BTREE_CORRUPTED = 3,

	/// Invalid arguments.
	CLOD_BTREE_INVALID = 4,

	/// The memory allocation callback returned null.
	CLOD_BTREE_ALLOC_FAILED = 5
};

#define CLOD_BTREE_VERSION 1

struct clod_btree {
	/// Value of the CLOD_BTREE_VERSION macro.
	int version;

	int _reserved;

	/// Root node.
	node *root_node;

	/// Mostly ignored for now - just used to validate pointer addresses.
	/// Maximum offset from the root node.
	size_t size;

	/// Allocator used to allocate nodes in the tree.
	clod_allocator allocator;

	/// Size of nodes in the tree.
	/// The allocator is only ever called with this.
	size_t node_size;

	/// Seed value used when performing node integrity checks.
	uint32_t check_seed;

	/// What node consistency checks should be performed.
	enum clod_btree_check check;

	/// Size of keys in the tree.
	unsigned char key_size;

	/// Size of values in the tree.
	unsigned char val_size;

	/// User variable passed to invocations of \p compare.
	void *compare_user;

	/// Comparison function.
	int (*compare)(const char *key1, const char *key2, void *user);
};

struct clod_btree_location {
	/// Value of the CLOD_BTREE_VERSION macro.
	int version;

	/// Index in the node of the element.
	int index;

	/// Node where the element is stored.
	node *node;

	/// Element key.
	const void *key;

	/// Element value.
	void *val;
};

/**
 * Initialise a btree.
 * @param[in] tree Tree to initialise.
 * @return Zer on success, or non-zero error code.
 * @throws CLOD_ERR_INVALID on invalid configuration options.
 */
CLOD_API CLOD_NONNULL(1)
enum clod_btree_result
clod_btree_create(struct clod_btree *tree);

/**
 * Free all resources associated with a btree.
 * @param[in] tree Tree to destroy.
 */
CLOD_API CLOD_NONNULL(1)
void
clod_btree_destroy(const struct clod_btree *tree);

/**
 * Inserts an element into the tree.
 * @param[in] tree Tree to insert the element into.
 * @param[in] key Key to insert.
 * @param[out] loc Element location.
 * @return Zero on success, or non-zero error code.
 * @throws CLOD_BTREE_OK On success.
 * @throws CLOD_BTREE_EXISTS The key compares equal to an existing key.
 * @throws CLOD_BTREE_CORRUPTED The tree appears corrupted.
 * @throws CLOD_BTREE_ALLOC_FAILED The memory allocator returned null.
 */
CLOD_API CLOD_NONNULL(1, 2, 3)
enum clod_btree_result
clod_btree_insert(const struct clod_btree *tree, void *key, struct clod_btree_location *loc);

/**
 * Get an element from the tree, optionally replacing its value.
 * @param[in] tree Tree to get the element from.
 * @param[in] key Key to find.
 * @param[out] loc Element location.
 * @return Zero on success, or non-zero error code.
 * @throws CLOD_BTREE_OK On success.
 * @throws CLOD_BTREE_NONEXIST The key does not compare equal to any key in the tree.
 * @throws CLOD_BTREE_CORRUPTED The tree appears corrupted.
 */
CLOD_API CLOD_NONNULL(1, 2)
enum clod_btree_result
clod_btree_find(const struct clod_btree *tree, void *key, struct clod_btree_location *loc);

/**
 * Removes an element from the tree.
 * @param[in] tree Tree to remove the element from.
 * @param[out] loc Element location.
 * @return Zero on success, or non-zero error code.
 * @throws CLOD_BTREE_OK On success.
 * @throws CLOD_BTREE_CORRUPTED The tree appears corrupted.
 */
CLOD_API CLOD_NONNULL(1, 2)
enum clod_btree_result
clod_btree_del(const struct clod_btree *tree, struct clod_btree_location *loc);

/**
 * Get the next element in the tree.
 * If the location is uninitialised, it returns the first element in the tree.
 * @param[in] tree Tree to iterate over.
 * @param[in,out] loc Element location.
 * @return Zero on success, or non-zero error code.
 * @throws CLOD_BTREE_OK On success.
 * @throws CLOD_BTREE_CORRUPTED The tree appears corrupted.
 * @throws CLOD_BTREE_NONEXIST A next element does not exist.
 */
CLOD_API CLOD_NONNULL(1, 2)
enum clod_btree_result
clod_btree_next(const struct clod_btree *tree, struct clod_btree_location *loc);

/**
 * Get the previous element in the tree.
 * If the location is uninitialised, it returns the last element in the tree.
 * @param[in] tree Tree to iterate over.
 * @param[in,out] loc Element location.
 * @return Zero on success, or non-zero error code.
 * @throws CLOD_BTREE_OK On success.
 * @throws CLOD_BTREE_CORRUPTED The tree appears corrupted.
 * @throws CLOD_BTREE_NONEXIST A previous element does not exist.
 */
CLOD_API CLOD_NONNULL(1, 2)
enum clod_btree_result
clod_btree_prev(const struct clod_btree *tree, struct clod_btree_location *loc);

/**
 * Check the entire tree for validity. It prints a debug message if enabled.
 * A completely empty (tree->root == nullptr) tree is considered valid.
 * @param[in] tree Tree to check for consistency.
 * @return Zero on success, non-zero error code on validation failure.
 * @throws CLOD_BTREE_OK On validation success.
 * @throws CLOD_BTREE_CORRUPTED On validation failure.
 */
CLOD_API CLOD_NONNULL(1)
enum clod_btree_result
clod_tree_validate(const struct clod_btree *tree);

/**
 * Print the tree in human-readable form to the given stream.
 * Useful for debugging.
 * @param[in] tree Tree to print.
 * @param[out] dst Stream to print to.
 * @param[in] max_depth Maximum depth to print to.
 * @return Error code returned by \p dst->write
 */
CLOD_API CLOD_NONNULL(1, 2)
int clod_tree_print(const struct clod_btree *tree, clod_stream *dst, int max_depth);

#endif
