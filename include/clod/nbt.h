/**
 * @file nbt.h
 * @defgroup nbt NBT
 * @{
 *
 * I've struggled to come up with a good public FFI-respecting API for this, and alas, I failed.
 * As such, this library is C-orientated and is unlikely to receive useful language bindings.
 * It does suck, because C is really good at this.
 * An optimising C compiler produces around ~100 instructions that parse NBTs at tens of GB/s -
 * perhaps the best non-hardware-specific NBT parsing method possible with modern technology.
 * Oh well.
 *
 * The NBT format is a serialised depth-first tree with no indexing.
 * The primary limitation of interacting with such a structure is simply figuring out where the nodes actually are.
 * To find a given NBT, every single tag before it must be recursively parsed to find the wanted NBT's offset.
 * The majority of the entire tree must be parsed for most operations.
 * As such, traversing the tree becomes the core limitation of any NBT operation and the primary focus for optimisation.
 * This traversal method is clod_nbt_payload_size.
 *
 * The library does not perform any kind of data validation.
 * Implementing complex data analysis to discern the likelihood that a given set of NBT data
 * has been modified from its original state is an insane alternative to using a checksum and out of scope for this library.
 * Please, for the love of good code, rely on dedicated verification methods instead of incidental parsing errors.
 * Notably, this non-feature is unrelated to memory safety, which this library does aim to provide.
 */
#ifndef CLOD_NBT_H
#define CLOD_NBT_H

#include <clod/lib.h>
#include <clod/big_endian.h>
#include <clod/sstr.h>
#include <stddef.h>

static_assert(CHAR_BIT == 8);

typedef signed char clod_nbt_type;
#define CLOD_NBT_ZERO        0
#define CLOD_NBT_INT8        1
#define CLOD_NBT_INT16       2
#define CLOD_NBT_INT32       3
#define CLOD_NBT_INT64       4
#define CLOD_NBT_FLOAT32     5
#define CLOD_NBT_FLOAT64     6
#define CLOD_NBT_INT8_ARRAY  7
#define CLOD_NBT_INT32_ARRAY 11
#define CLOD_NBT_INT64_ARRAY 12
#define CLOD_NBT_STRING      8
#define CLOD_NBT_LIST        9
#define CLOD_NBT_COMPOUND    10

/** Named Binary Tag. */
union clod_nbt_tag {
	struct {
		/// Type of the NBT.
		clod_nbt_type type;

		/// Size of the name of the tag.
		beu16 name_size;

		/// Name of the tag. Not zero delimited.
		char name[/* name_size */];

		/// payload follows
	};
};

static_assert(alignof(union clod_nbt_tag) == 1);
static_assert(sizeof(union clod_nbt_tag) == 3);

/** Union of all NBT payload types. */
union clod_nbt_payload {
	/// NBT Byte payload
	bei8 int8;

	/// NBT Short payload
	bei16 int16;

	/// NBT Int payload
	bei32 int32;

	/// NBT Long payload
	bei64 int64;

	/// NBT Float payload
	bef32 float32;

	/// NBT Double payload
	bef64 float64;

	/// NBT Byte Array payload
	struct {
		bei32 length;
		bei8 data[/* length */];
	} byte_array;

	/// NBT String payload
	struct {
		beu16 length;
		char data[/* length */];
	} string;

	/// NBT List payload
	struct {
		clod_nbt_type payload_type;
		bei32 length;
		char data[/* length */];
	} list;

	/// NBT Compound payload
	/// Delimited by a CLOD_NBT_ZERO tag of size 1.
	union {
		/// The delimiting CLOD_NBT_ZERO element has a size of 1.
		clod_nbt_type delimiter;
		union clod_nbt_tag payload/*[]*/;
	} compound;

	/// Int Array
	struct {
		bei32 length;
		bei32 data[/* length */];
	} int32_array;

	/// Long Array
	struct {
		bei32 length;
		bei64 data[/* length */];
	} int64_array;
};

static_assert(alignof(union clod_nbt_payload) == 1);

/**
 * Get the size of an NBT payload.
 * This is the primary NBT traversing function; everything else is built on top of this.
 *
 * @param[in] payload The payload to get the size of.
 * @param[in] payload_type The type of the payload.
 * @param[in] end Point past which the method will never read.
 * @return The size of the payload, or 0 on invalid arguments.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 3)
size_t
clod_nbt_payload_size(
	const union clod_nbt_payload *payload,
	clod_nbt_type payload_type,
	const char *end
);

/**
 * Get the size of an NBT tag.
 * It's mostly a wrapper around clod_nbt_payload_size.
 *
 * @param[in] tag The tag to get the size of.
 * @param[in] end Point past which the method will never read.
 * @return Size of the tag, or 0 on invalid arguments.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 2)
size_t
clod_nbt_tag_size(
	const union clod_nbt_tag *tag,
	const char *end
);

/**
 * Get a tag's payload.
 *
 * @param[in] tag The tag to get the payload for.
 * @param[in] end Point past which the method will never read.
 * @param[in] payload_type The type the payload is expected to be.
 * This is functionally unnecessary but avoids easy accidental misuse that can cause unsafe memory access.
 * If the payload doesn't match, then null is returned and the error (more) safely propagates.
 * @return The tag's payload, or nullptr on buffer overrun.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 2)
union clod_nbt_payload *
clod_nbt_tag_payload(
	const union clod_nbt_tag *tag,
	const char *end,
	clod_nbt_type payload_type
);

/**
 * Get, create, or delete an element in a compound payload.
 *
 * If \p compound is null,
 * \p end_offset is incremented by the worst case (largest) increase in NBT data size
 * the operation could produce, and the method returns null.
 *
 * @param[in] compound (nullable) The compound payload.
 * @param[in,out] end The end of NBT data. Updated to reflect any changes made.
 * @param[in,out] free (nullable) The amount of free space. Updated to reflect any changes made.
 * A positive value indicates how much free space exists that \p end can be incremented into,
 * and a negative value indicates that changes would have overrun the buffer and no changes were made.
 * If null, the operation never modifies the NBT data.
 * @param[in] type If element creation occurs, the type it should be created with,
 * or CLOD_NBT_ZERO to delete the tag.
 * @param[in] name Name of the element.
 * @return The element tag.
 * Null if changes would overrun the buffer, on deletion,
 * or if \p free is null and the element doesn't exist.
 */
CLOD_API CLOD_NONNULL(2)
union clod_nbt_tag *
clod_nbt_compound(
	union clod_nbt_payload *compound,
	const char **end,
	ptrdiff_t *free,
	clod_nbt_type type,
	clod_sstr name
);

/**
 * Get an element in a compound payload.
 *
 * @param[in] compound The compound payload.
 * @param[in] end Point past which the method will never read.
 * @param[in] name Name of the child tag.
 * @return The child tag, or null on buffer overrun or nonexistent child.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 2)
union clod_nbt_tag *
clod_nbt_compound_get(const union clod_nbt_payload *compound, const char *end, clod_sstr name);

/**
 * Get an element in a list payload.
 *
 * @param[in] list The list payload.
 * @param[in] end Point past which the method will never read.
 * @param[in] index List index to get.
 * @return The payload at the provided index, or null on buffer overrun or index out of bounds.
 */
CLOD_API CLOD_PURE CLOD_NONNULL(1, 2)
union clod_nbt_payload *
clod_nbt_list_get(const union clod_nbt_payload *list, const char *end, size_t index);

/**
 * Modify a list payload's length.
 *
 * @param[in] list (nullable) The list payload.
 * @param[in,out] end The end of NBT data.
 * If NBT data is mutated, it is updated to reflect the new size.
 * @param[in,out] end_offset The offset of \p end relative to the end of the buffer.
 * A negative value indicates there is free space that end can be grown into,
 * and a positive value indicates a buffer overrun.
 * @param[in] type The type of list elements.
 * @param[in] index The number of list elements.
 * @return The first list element.
 */
CLOD_API CLOD_NONNULL(2, 3)
union clod_nbt_payload *
clod_nbt_list_set(
	union clod_nbt_payload *list,
	const char **end,
	ptrdiff_t *end_offset,
	clod_nbt_type type,
	size_t index
);


/** @} */
#endif
