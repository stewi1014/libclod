/**
 * @file nbt.h
 * @defgroup nbt NBT
 * @{
 *
 * The NBT format is a serialised depth-first tree with no indexing.
 * The primary limitation of interacting with such a structure is simply figuring out where the nodes actually are.
 * To find a given NBT, every single tag before it must be recursively parsed to find the wanted NBT's offset.
 * The majority of the entire tree must be parsed for most operations.
 * As such, traversing the tree becomes the core limitation of any NBT operation and the primary focus for optimisation.
 * This traversal method is clod_nbt_payload_size.
 *
 * The library only performs some basic sanity checks on top of ensuring memory safety.
 * Implementing complex data analysis to discern the likelihood that a given set of NBT data
 * has been modified from its original state is an insane alternative to using a checksum and out of scope for this library.
 * Please, for the love of good code, rely on dedicated verification methods instead of incidental parsing errors.
 */
#ifndef CLOD_NBT_H
#define CLOD_NBT_H

#include <clod/lib.h>
#include <clod/big_endian.h>
#include <clod/sstr.h>
#include <stddef.h>

static_assert(CHAR_BIT == 8);

#define CLOD_NBT_ZERO        (char)(0)
#define CLOD_NBT_INT8        (char)(1)
#define CLOD_NBT_INT16       (char)(2)
#define CLOD_NBT_INT32       (char)(3)
#define CLOD_NBT_INT64       (char)(4)
#define CLOD_NBT_FLOAT32     (char)(5)
#define CLOD_NBT_FLOAT64     (char)(6)
#define CLOD_NBT_INT8_ARRAY  (char)(7)
#define CLOD_NBT_INT32_ARRAY (char)(11)
#define CLOD_NBT_INT64_ARRAY (char)(12)
#define CLOD_NBT_STRING      (char)(8)
#define CLOD_NBT_LIST        (char)(9)
#define CLOD_NBT_COMPOUND    (char)(10)

#define CLOD_NBT_ROOT_COMPOUND_INIT   ((char[]){CLOD_NBT_COMPOUND, 0, 0, 0})
#define CLOD_NBT_ROOT_LIST_INIT(type) ((char[]){CLOD_NBT_LIST, type, 0, 0, 0, 0})

/**
 * Get the size of a payload.
 * This is the primary NBT traversing function; everything else is built on top of this.
 *
 * @param[in] payload The payload to get the size of.
 * @param[in] payload_type The type of the payload.
 * @param[in] end End of the NBT data.
 * @return The size of the payload, or 0 on failure.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 2)
size_t clod_nbt_payload_size(
	const char *restrict payload,
	const void *end,
	char payload_type
);

/**
 * Get the size of a tag including its payload.
 *
 * @param[in] tag The tag to get the size of.
 * @param[in] end End of the NBT data.
 * @return Size of the tag, or 0 on failure.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1)
size_t clod_nbt_tag_size(const char *restrict tag, const void *end);

/**
 * Get a tag's payload.
 *
 * @param[in] tag The tag to get the payload of.
 * @param[in] end Point past which the method will never read.
 * @return The tag's payload.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 2)
char *clod_nbt_tag_payload(const char *restrict tag, const void *end);

/**
 * Get the name of a tag.
 *
 * @param[in] tag The tag to get the name of.
 * @param[in] end Point past which the method will never read.
 * @return The tag's name.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 2)
clod_sstr clod_nbt_tag_name(const char *tag, const void *end);

/**
 * Iterator.
 * Users must not modify Fields during iteration.
 * A zeroed payload field is used to start an iteration.
 */
struct clod_nbt_iter {
	/** The tag, if one exists.
	 * It points to the byte following the last iterated payload when iteration ends. */
	char *tag;
	/** The payload. A null value starts iteration. */
	char *payload;
	/** Size of payload, or tag + payload if tag exists. */
	size_t size;
	/** Type of the payload. */
	char type;
	/** The index of the payload. Equal to the total number of elements after the iteration ends. */
	uint32_t index;
};
#define CLOD_NBT_ITER_ZERO { .type = CLOD_NBT_ZERO }

/**
 * Iterate over elements in a payload.
 *
 * @param[in] payload The payload whose elements are to be iterated over.
 * @param[in] end End of NBT data.
 * @param[in] payload_type Type of the payload.
 * An invalid payload_type makes the function a false-returning no-op.
 * @param[in,out] iter Iterator.
 * Upon completion (false return), \p iter.tag is always set to
 * @return True if the last element was found,
 * false if it was not.
 * A false return and null \p iter.payload field indicates error.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2, 4)
bool clod_nbt_iter_next(
	const char *restrict payload,
	const void *end,
	char payload_type,
	struct clod_nbt_iter *iter
);

/**
 * Get an element in a compound payload.
 *
 * @param[in] compound Payload to find element in.
 * @param[in] end End of the NBT data.
 * @param[in] name Name of the element.
 * @return Element tag, or null if none was found.
 */
CLOD_API CLOD_NONNULL(1, 2)
char *clod_nbt_compound_get(
	const char *restrict compound,
	const void *end,
	clod_sstr name
);

/**
 * Get or create an element in a compound payload.
 *
 * @param[in] compound Payload to find or create element in.
 * If \p compound is null, then the size that would be written on creation is subtracted from free.
 * @param[in,out] end End of the NBT data.
 * @param[in,out] free Free space in the buffer.
 * It is modified to reflect the change in NBT data size.
 * A negative value after return indicates the writing failed due to lack of space.
 * @param[in] name Name of the element to search for.
 * @param[in] type Type of the new element if creation occurs.
 * @return Element tag if one was found,
 * the created element if it was created,
 * or null if there isn't enough free space.
 */
CLOD_API CLOD_NONNULL(2, 3)
char *clod_nbt_compound_add(
	char *restrict compound,
	const void **end,
	ptrdiff_t *free,
	clod_sstr name,
	char type
);

/**
 * Delete an element in a compound payload.
 *
 * @param[in] compound Payload to delete element in.
 * @param[in,out] end End of the NBT data.
 * @param[in,out] free Free space in the buffer.
 * @param[in] name Name of the element to delete.
 * @return True on success, false on failure.
 */
CLOD_API CLOD_NONNULL(1, 2, 3)
bool clod_nbt_compound_del(
	char *restrict compound,
	const void **end,
	ptrdiff_t *free,
	clod_sstr name
);

/**
 * Resize a list payload.
 *
 * @param[in] list The list payload to resize.
 * @param[in,out] end End of NBT data.
 * @param[in,out] free Free space in the buffer.
 * @param[in] type If non-zero, the type of list elements will be set to this.
 * Changing types forces wiping all existing elements in the list.
 * @param[in] length New length.
 * @return True on success, false on failure.
 */
CLOD_API CLOD_NONNULL(2)
bool clod_nbt_list_resize(
	char *restrict list,
	const char **end,
	ptrdiff_t *free,
	char type,
	uint32_t length
);

/** @} */
#endif
