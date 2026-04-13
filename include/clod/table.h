/**
 * @file clod/table.h
 * @defgroup table Hash Table
 *
 * A hash table implementation.
 * It's a pure set at heart, but since methods never return a pointer not supplied by the user,
 * it can be trivially extended to a key->value map by storing the value after the key.
 * The difference in wording between 'element' and 'key' reflects this.
 * If implementing a map, 'element' implies the key and trailing value data associated with it,
 * while 'key' simply implies the key value.
 * If instead a pure set is required - 'element' and 'key' become interchangeable.
 * I would recommend using a struct with the first field being the key and subsequent fields being the value,
 * although for maps with variable-length keys more manual intervention may be required.
 *
 * @{
 */
#ifndef LIBCLOD_TABLE_H
#define LIBCLOD_TABLE_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @struct clod_table
 * Open-addressed hash table.
 */
struct clod_table;

/**
 * Configuration options passed to clod_table_create.
 * Zero values imply defaults.
 */
struct clod_table_opts {
	/** Minimum number of elements to support without further allocation.
	 * The table will never shrink to a capacity below this. */
	size_t min_capacity;
	/** Custom hash function. Defaults to clod_sip64.
	 * This map implementation demands a uniform spread of entropy across _all_ bits in uint64,
	 * and that two keys with nonequal hashes are also not equal when using the cmp_func method.*/
	uint64_t (*hash_func)(uint64_t seed, const void *key, size_t key_size, void *user);
	/** Custom equality function. Defaults to memcmp.
	 * The required behaviour for the hash_func and cmp_func relationship is;
	 * assert(cmp_func(a, b) != 0 || hash_func(a) == hash_func(b));
	 * In other words, if cmp_func thinks two elements are equal, hash_func must agree.*/
	int (*cmp_func)(const void *key1, size_t key1_size, const void *key2, size_t key2_size, void *user);
	/** Custom memory allocation function. */
	void *(*malloc_func)(size_t, void *user);
	/** Custom memory freeing function. */
	void (*free_func)(void*, void *user);
	/** Value passed to callbacks. */
	void *user;
};

/**
 * Create a new table.
 * @param[in] opts Configuration options for the table.
 * @return Initialised table.
 */
CLOD_API CLOD_USE_RETURN
struct clod_table *
clod_table_create(const struct clod_table_opts *opts);

/**
 * Release resources associated with the table.
 * @param t Table to free.
 */
CLOD_API CLOD_NONNULL(1)
void
clod_table_destroy(struct clod_table *t);

/**
 * Get the number of elements in the table.
 * @param[in] t Handle to the table.
 * @return Number of elements currently in the table.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1)
size_t
clod_table_len(const struct clod_table *t);

/**
 * Add an element.
 * If the key already exists the operation fails.
 * A false return and null \p existing_out indicates an allocation failure.
 *
 * @param[in] t Handle to the table.
 * @param[in] element Element to insert. The table takes ownership on success.
 * @param[in] key_size Size of the key.
 * @param[out] existing_out (nullable) Returns the existing element if the key already existed.
 * The table retains ownership. Returns null on allocation failure.
 * @return True on success. False if the key already existed, or on allocation failure.
 */
CLOD_API CLOD_NONNULL(1, 2)
bool
clod_table_add(struct clod_table *t, const void *element, size_t key_size, void **existing_out);

/**
 * Add or replace an element.
 * If the key already exists, it is replaced.
 * A false return and null \p existing_out indicates allocation failure.
 *
 * @param[in] t Handle to the table.
 * @param[in] element Element to insert. The table takes ownership on success.
 * @param[in] key_size Size of the key.
 * @param[out] existing_out Returns the existing element if the key already existed.
 * The caller takes ownership on success. Returns null on allocation failure.
 * @return True on success. False on allocation failure.
 */
CLOD_API CLOD_NONNULL(1, 2, 4)
bool
clod_table_set(struct clod_table *t, const void *element, size_t key_size, void **existing_out);

/**
 * Get an element from the table.
 * @param[in] t Handle to the table.
 * @param[in] key Key to look up. Caller retains ownership.
 * @param[in] key_size Size of the key.
 * @return Found element or null. The table retains ownership of the element.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 2)
void *
clod_table_get(const struct clod_table *t, const void *key, size_t key_size);

/**
 * Remove an element from the table.
 * @param[in] t Handle to the table.
 * @param[in] key Key to delete. Caller retains ownership.
 * @param[in] key_size Size of the key.
 * @return The removed element, or nullptr if the key doesn't exist.
 * The caller takes ownership of the element.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2)
void *
clod_table_del(struct clod_table *t, const void *key, size_t key_size);

struct clod_table_iter {
	void *element;
	size_t key_size;
	size_t _internal;
};
#define CLOD_TABLE_ITER_INIT (struct clod_table_iter){ ._internal = 0 }

/**
 * Get the next element in, or start, an iteration over table elements.
 * Mutating the table, except deleting elements, during iteration can
 * result in existing elements being iterated more than once or not at all.
 *
 * The iterator should be zero initialised to start an iteration.
 * The iterator is zeroed at the end of the iteration.
 *
 * @param[in] t Handle to the table.
 * @param[in,out] iter Iterator to be incremented.
 * @return If the next element was found.
 */
CLOD_API CLOD_NONNULL(1, 2)
bool
clod_table_iter(const struct clod_table *t, struct clod_table_iter *iter);

/**
 * A hash method that hashes the pointer value itself instead of the data it references.
 * Provided here as a helper for using the table as a pointer set.
 */
CLOD_API
uint64_t
clod_table_hash_ptr(uint64_t seed, const void *key, size_t key_size, void *user);

/**
 * A comparison method that compares the pointer values themselves instead of the data they reference.
 * Provided here as a helper for using the table as a pointer set.
 */
CLOD_API
int
clod_table_cmp_ptr(const void *key1, size_t key1_size, const void *key2, size_t key2_size, void *user);

/** @} */
#endif
