#ifndef LIBCLOD_ALLOCATOR_H
#define LIBCLOD_ALLOCATOR_H

#include "clod_config.h"
#include <clod/memory.h>
#include <clod/hash.h>

#include "clod/debug.h"

#define BLOCK_SIZE 256
#define TREE_ORDER 31
#define TREE_MAX_DEPTH 5

struct node;
struct branch;

/// Block Pointer

typedef uint32_t block_ptr;

#define IS_BLOCK_ALIGNED(n) ((uintptr_t)(n) % BLOCK_SIZE == 0)
#define BLOCK_PTR_NULL ((block_ptr)0)

#define TYPE_NODE   1
#define TYPE_SPAN   2
#define TYPE_SLAB   3
#define TYPE_BRANCH 4

#define SLAB_1      0
#define SLAB_2      1
#define SLAB_4      2
#define SLAB_8      3
#define SLAB_12_4   4
#define SLAB_16     5
#define SLAB_24_8   6
#define SLAB_32     7
#define SLAB_48_16  8
#define SLAB_64     9
#define SLAB_96_32  10
#define SLAB_128    11
#define SLAB_192_64 12
#define SLAB_CLASS_COUNT 13

static block_ptr node_ptr(clod_allocator *head, struct node *node) {
	#if CLOD_MEMORY_DEBUG
		if ((char*)node < (char*)head) return BLOCK_PTR_NULL;
		if (!IS_BLOCK_ALIGNED(head) || !IS_BLOCK_ALIGNED(node)) return BLOCK_PTR_NULL;
	#endif
	return (block_ptr)((char*)node - (char*)head) + TYPE_NODE;
}
static block_ptr span_ptr(clod_allocator *head, char *span) {
	#if CLOD_MEMORY_DEBUG
		if ((char*)span < (char*)head) return BLOCK_PTR_NULL;
		if (!IS_BLOCK_ALIGNED(head) || !IS_BLOCK_ALIGNED(span)) return BLOCK_PTR_NULL;
	#endif
	return (block_ptr)((char*)span - (char*)head) + TYPE_SPAN;
}
static block_ptr slab_ptr(clod_allocator *head, char *slab, const int slab_class) {
	#if CLOD_MEMORY_DEBUG
		if (1 > slab_class || slab_class > 13) return BLOCK_PTR_NULL;
		if ((char*)slab < (char*)head) return BLOCK_PTR_NULL;
		if (!IS_BLOCK_ALIGNED(head) || !IS_BLOCK_ALIGNED(slab)) return BLOCK_PTR_NULL;
	#endif
	return (block_ptr)((char*)slab - (char*)head + TYPE_SLAB + (slab_class << 4));
}
static block_ptr branch_ptr(clod_allocator *head, struct node *node, const int index) {
	#if CLOD_MEMORY_DEBUG
		if ((char*)node < (char*)head) return BLOCK_PTR_NULL;
		if (!IS_BLOCK_ALIGNED(node) || !IS_BLOCK_ALIGNED(head)) return BLOCK_PTR_NULL;
		if (0 > index || index >= TREE_ORDER) return BLOCK_PTR_NULL;
	#endif
	return (block_ptr)((char*)node - (char*)head) + TYPE_BRANCH + ((block_ptr)index << 3);
}

static int ptr_type(const block_ptr bptr) {
	if ((bptr & 0xFF) == TYPE_NODE) return TYPE_NODE;
	if ((bptr & 0xFF) == TYPE_SPAN) return TYPE_SPAN;
	if ((bptr & 0x0F) == TYPE_SLAB) return TYPE_SLAB;
	if ((bptr & 0x07) == TYPE_BRANCH) return TYPE_BRANCH;
	return 0;
}
static uint32_t block_offset(const block_ptr bptr) { return bptr & 0xFFFFFF00; }

static struct node *node_get(clod_allocator *head, const block_ptr bptr) {
	if ((bptr & 0xFF) != TYPE_NODE && (bptr & 0x7) != TYPE_BRANCH) return nullptr;
	return (struct node*)((char*)head + block_offset(bptr));
}
static char *span_get(clod_allocator *head, const block_ptr bptr) {
	if ((bptr & 0xFF) != TYPE_SPAN) return nullptr;
	return (char*)head + block_offset(bptr);
}
static char *slab_get(clod_allocator *head, const block_ptr bptr) {
	if ((bptr & 0xF) != TYPE_SLAB) return nullptr;
	return (char*)head + block_offset(bptr);
}
static int slab_type_get(const block_ptr bptr) {
	if ((bptr & 0xF) != TYPE_SLAB) return 0;
	int type = (bptr & 0xF0) >> 4;
	#if CLOD_MEMORY_DEBUG
		if (1 > type || type > 13) return 0;
	#endif
	return type;
}
static int branch_index_get(const block_ptr bptr) {
	if ((bptr & 0x7) != TYPE_BRANCH) return 0;
	return (bptr & 0xF8) >> 3;
}

struct branch {
	/// Either the next node in the tree or the block value.
	block_ptr ptr;

	/// Meaning depends on the type of branch, which is stored in \p ptr.
	///
	/// If \p ptr points to another node, it stores the actual key for
	/// lookups, as the location of the node in memory does not
	/// correlate with the location of the elements it's storing. Otherwise,
	/// \p ptr points to the actual block/blocks stored in the tree, and
	/// their location is the key.
	///
	/// If \p ptr points to a slab, it contains a used/free bitfield of
	/// elements in the slab. For split slabs 32 bits is not large enough,
	/// so represents groupings of elements instead of individual ones,
	/// and the individual use/free flags are stored in the slab block itself.
	///
	/// If \p ptr points to a span, it contains the size of the span in bytes.
	uint32_t data;
};

/// Key is the location of the block/blocks.
/// Value is the location of the block/blocks and some metadata.
struct node {
	/// Checksum
	uint32_t checksum;

	alignas(8) struct branch branches[TREE_ORDER];
};

static_assert(sizeof(struct node) <= 256);

typedef struct branch **path;
#define PATH_NEW (&(struct branch*[TREE_MAX_DEPTH + 1]){nullptr}[1])
static void path_push(path *path, struct branch *branch) { *++*path = branch; }
static struct branch *path_pop(path *path) { return **path == nullptr ? nullptr : *--*path; }
static struct branch *path_get(const path path, const int parent) {
	for (int i = 0; i < parent; i++)
		if (path[-i] == nullptr) return nullptr;
	return path[-parent];
}

bool tree_next(clod_allocator *head, path *path);
bool tree_prev(clod_allocator *head, path *path);
static bool tree_iter(clod_allocator *head, struct node *root_node, path *path) {
	if (path_get(*path, 0) == nullptr) {
		path_push(path, root_node->branches);
	}

	return tree_next(head, path);
}

void tree_search(clod_allocator *head, struct node *root_node, block_ptr key, path *path);
struct branch *tree_add(clod_allocator *head, struct node *root_node, struct branch branch, path *path);
void free_tree_add(clod_allocator *head, struct node *root_node, struct branch branch);
void used_tree_add(clod_allocator *head, struct node *root_node, struct branch branch);

/// Allocator

struct clod_allocator {
	/// Checksum
	uint32_t checksum;

	/// Size of backing memory.
	uint32_t size;

	/// The allocator may need to create more than one backing memory instance.
	/// This forms a circular linked list between all of them.
	clod_allocator *next;

	/// Pointer to the tree branch containing the slab that was most recently used
	/// to fulfill an allocation. Allows repeated slab-sized allocations to (usually)
	/// quickly find a slab with free space instead of searching the used tree.
	///
	/// They should not blindly be used, as the tree may have been restructured.
	/// So the usual checks as done when searching the used tree for slabs should
	/// still be performed.
	block_ptr last_slab_branch[SLAB_CLASS_COUNT];

	alignas(BLOCK_SIZE) struct node free;
	alignas(BLOCK_SIZE) struct node used;
};

/// CRC

#define METADATA_CRC_INIT UINT32_C(0xAE11F296)
static void crc_check(void *block) {
	#if CLOD_MEMORY_DEBUG
	uint32_t crc = clod_crc32_add(METADATA_CRC_INIT, (char*)block + sizeof(uint32_t), BLOCK_SIZE - sizeof(uint32_t));
	crc = clod_crc32_add(crc, &block, sizeof(block));
	#else
	uint32_t crc = (uint32_t)block;
	#endif

	if (*(uint32_t*)block != crc) {
		fatal_old(CLOD_MEMORY_DEBUG, "Allocator metadata is corrupted.");
	}
}
static void crc_update(void *block) {
	#if CLOD_MEMORY_DEBUG
	uint32_t crc = clod_crc32_add(METADATA_CRC_INIT, (char*)block + sizeof(uint32_t), BLOCK_SIZE - sizeof(uint32_t));
	crc = clod_crc32_add(crc, &block, sizeof(block));
	#else
	uint32_t crc = (uint32_t)block;
	#endif

	*(uint32_t*)block = crc;
}

#endif
