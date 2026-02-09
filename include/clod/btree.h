/**
 * @file clod/btree.h
 * @defgroup btree B+Tree
 * @{
 *
 * A concurrent B+Tree implementation designed for storage.
 * It maps a key to a value with a maximum size of 256 for both.
 * Zero-size values are supported and the zero-size key is valid.
 *
 * The tree requires an alignment of 4 and uses a page size of 4096.
 * A new tree must have a size of at least 4096 bytes,
 * and is initialised by zeroing at least the first 4096 bytes.
 */
#ifndef CLOD_BTREE_H
#define CLOD_BTREE_H

#include <clod/lib.h>
#include <stdint.h>

enum clod_btree_result {
	/** No worries. */
	CLOD_BTREE_OK = 0,
	/** Element can't be added as there is not enough space,
	 * or the provided buffer is smaller than a single page. */
	CLOD_BTREE_NO_SPACE = 1,
	/** The btree appears malformed. */
	CLOD_BTREE_MALFORMED = 2,
	/** The key does not exist. */
	CLOD_BTREE_NOT_EXIST = 3,
	/** The provided btree pointer is not aligned correctly. */
	CLOD_BTREE_NOT_ALIGNED = 4,
	/** The btree is currently busy being written to.
	 * Methods typically wait instead of returning this. */
	CLOD_BTREE_BUSY = 5,
	/** Iteration ended. */
	CLOD_BTREE_END = 6
};

/**
 * Get a value for a given key.
 *
 * Notably, this is the
 *
 * @param btree
 * @param btree_end
 * @param key
 * @param key_size
 * @param value
 * @param value_size
 * @return
 */
CLOD_API CLOD_NONNULL(1, 3)
enum clod_btree_result clod_btree_get(
	const uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size,
	uint8_t *value, uint8_t *value_size
);

CLOD_API CLOD_NONNULL(1, 3, 5)
enum clod_btree_result clod_btree_set(
	uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size,
	const uint8_t *value, uint8_t value_size
);

CLOD_API CLOD_NONNULL(1, 3)
enum clod_btree_result clod_btree_del(
	uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size
);

CLOD_API CLOD_NONNULL(1, 3)
enum clod_btree_result clod_btree_size(
	const uint8_t *btree, const uint8_t *btree_end,
	size_t *btree_size_out
);

CLOD_API CLOD_NONNULL(1, 5, 6)
enum clod_btree_result clod_btree_next(
	uint8_t *btree, const uint8_t *btree_end,
	const uint8_t *key, uint8_t key_size,
	const uint8_t **next_key, uint8_t *next_key_size,
	const uint8_t **next_value, uint8_t *next_value_size
);


/** @} */
#endif
