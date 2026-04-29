#include <clod/memory.h>
#include <clod/btree.h>
#include <clod/string.h>
#include <clod/stream/stream.h>
#include <clod/sys/vm.h>
#include "debug.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define TAG_BITS 2
#define TAG_CONTIGUOUS 1
#define TAG_DEDICATED 2

#define ENCODE_CONTIGUOUS(size) (((size) << TAG_BITS) | TAG_CONTIGUOUS)
#define ENCODE_DEDICATED(size) (((size) << TAG_BITS) | TAG_DEDICATED)
#define DECODE_TAG(val) ((int)((val) & ((1 << TAG_BITS) - 1)))
#define DECODE_VALUE(val) ((val) >> TAG_BITS)

#define NODE_SIZE 512
#define MIN_BLOCK_SIZE 1024
#define HEADER_BLOCKS 3

struct clod_allocator_impl {
    clod_allocator base;
    struct clod_btree free_tree;
    struct clod_btree used_tree;
    size_t page_size;
    size_t dedicated_vm_threshold;
    bool no_grow;
    void *vm_user;
    void *(*vm_alloc)(void *user, size_t size);
    void (*vm_free)(void *user, void *ptr, size_t size);
    void *backing_start;
    size_t backing_size;
};

static void *allocate_fn(clod_allocator *self, size_t size);
static void free_fn(clod_allocator *self, void *ptr);

static void *default_vm_alloc(void *user, size_t size) {
    (void)user;
    return clod_vm_alloc(size);
}

static void default_vm_free(void *user, void *ptr, size_t size) {
    (void)user;
    clod_vm_free(ptr, size);
}

/**
 * Tree node allocator that allocates directly from vm_alloc to avoid recursion.
 * Tree nodes are never freed individually - they're reclaimed when the tree is destroyed.
 */
static void *tree_node_alloc(clod_allocator *self, size_t size) {
    (void)self;
    (void)size;
    return clod_vm_alloc(NODE_SIZE);
}

static void tree_node_free(clod_allocator *self, void *ptr) {
    (void)self;
    (void)ptr;
    // Tree nodes are not freed individually - they're reclaimed when the tree is destroyed
}

static clod_allocator tree_node_allocator = { .allocate = tree_node_alloc, .free = tree_node_free };

static size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static size_t blocks_for_size(size_t size) {
    return (size + MIN_BLOCK_SIZE - 1) / MIN_BLOCK_SIZE;
}

/**
 * Grow the free pool by allocating more backing memory.
 */
static bool grow_pool(struct clod_allocator_impl *impl, size_t blocks_needed) {
    if (impl->no_grow) return false;

    size_t grow_pages = align_up(blocks_needed * MIN_BLOCK_SIZE, impl->page_size) / impl->page_size;
    if (grow_pages == 0) grow_pages = 16;

    void *ptr = impl->vm_alloc(impl->vm_user, grow_pages * impl->page_size);
    if (!ptr) return false;

    uint64_t addr = (uint64_t)ptr;
    uint64_t size_blocks = blocks_for_size(grow_pages * impl->page_size);
    
    // Insert into free tree - uses special allocator that doesn't recurse
    uint64_t *val_ptr = clod_btree_insert(&impl->free_tree, addr);
    if (!val_ptr) {
        impl->vm_free(impl->vm_user, ptr, grow_pages * impl->page_size);
        return false;
    }
    *val_ptr = size_blocks;
    
    return true;
}

/**
 * Allocate a region from the free tree.
 */
static void *do_allocate(struct clod_allocator_impl *impl, size_t size) {
    if (size == 0) return (void *)1;

    // Handle dedicated VM allocations for large requests
    if (size >= impl->dedicated_vm_threshold) {
        size_t aligned = align_up(size, impl->page_size);
        void *ptr = impl->vm_alloc(impl->vm_user, aligned);
        if (!ptr) return NULL;
        uint64_t addr = (uint64_t)ptr;
        uint64_t encoded = ENCODE_DEDICATED(aligned);
        uint64_t *val_ptr = clod_btree_insert(&impl->used_tree, addr);
        if (!val_ptr) {
            impl->vm_free(impl->vm_user, ptr, aligned);
            return NULL;
        }
        *val_ptr = encoded;
        return ptr;
    }

    size_t needed_blocks = blocks_for_size(size);

    // Find a suitable free region
    uint64_t free_addr = 0, free_blocks = 0;
    struct clod_btree_iter iter = CLOD_BTREE_ITER_INIT;

    while (clod_btree_iter_next(&impl->free_tree, &iter, &free_addr, &free_blocks)) {
        if (free_blocks >= needed_blocks) {
            // Found a suitable region
            uint64_t use_blocks = needed_blocks;
            uint64_t remaining = free_blocks - use_blocks;

            clod_btree_delete(&impl->free_tree, free_addr, NULL);

            // Put back any remaining space
            if (remaining > 0) {
                uint64_t new_addr = free_addr + use_blocks * MIN_BLOCK_SIZE;
                uint64_t *val_ptr = clod_btree_insert(&impl->free_tree, new_addr);
                if (val_ptr) *val_ptr = remaining;
            }

            // Mark as used
            uint64_t encoded = ENCODE_CONTIGUOUS(needed_blocks * MIN_BLOCK_SIZE);
            uint64_t *val_ptr = clod_btree_insert(&impl->used_tree, free_addr);
            if (val_ptr) *val_ptr = encoded;
            return (void *)free_addr;
        }
    }

    // No suitable region found, try to grow
    if (grow_pool(impl, needed_blocks)) {
        return do_allocate(impl, size);
    }

    return NULL;
}

int clod_allocator_create(clod_allocator **allocator_out, const struct clod_allocator_opts *opts) {
    size_t impl_size = align_up(sizeof(struct clod_allocator_impl), MIN_BLOCK_SIZE);
    size_t header_total = HEADER_BLOCKS * MIN_BLOCK_SIZE;

    size_t initial_pages = 1024;
    if (opts && opts->initial_pages > 0) {
        initial_pages = opts->initial_pages;
    }

    size_t backing_size = initial_pages * clod_vm_page_size();
    void *backing = clod_vm_alloc(backing_size);
    if (!backing) return 1;

    struct clod_allocator_impl *impl = backing;
    impl->base.allocate = allocate_fn;
    impl->base.free = free_fn;

    impl->page_size = opts && opts->page_size ? opts->page_size : clod_vm_page_size();
    impl->dedicated_vm_threshold = opts && opts->dedicated_vm_threshold ? opts->dedicated_vm_threshold : 64 * 1024 * 1024;
    impl->no_grow = opts ? opts->no_grow : false;
    impl->vm_user = opts ? opts->vm_user : NULL;
    impl->vm_alloc = opts && opts->vm_alloc ? opts->vm_alloc : default_vm_alloc;
    impl->vm_free = opts && opts->vm_free ? opts->vm_free : default_vm_free;
    impl->backing_start = backing;
    impl->backing_size = backing_size;

    // Initialize trees
    // Root nodes are at fixed positions: block 1 = free tree root, block 2 = used tree root
    // Block 0 is the allocator impl structure
    struct clod_btree_node *free_root = (struct clod_btree_node *)((uintptr_t)backing + impl_size);
    struct clod_btree_node *used_root = (struct clod_btree_node *)((uintptr_t)free_root + sizeof(struct clod_btree_node));
    void *data_start = (void *)((uintptr_t)used_root + sizeof(struct clod_btree_node));

    // Initialize root nodes in place (they are at fixed addresses)
    free_root->is_leaf = true;
    free_root->count = 0;
    free_root->parent = NULL;
    for (int i = 0; i < CLOD_BTREE_ORDER; i++) free_root->u.data.children[i] = NULL;
    for (int i = 0; i < CLOD_BTREE_ORDER - 1; i++) { free_root->u.keys[i] = 0; free_root->u.data.values[i] = 0; }
    
    used_root->is_leaf = true;
    used_root->count = 0;
    used_root->parent = NULL;
    for (int i = 0; i < CLOD_BTREE_ORDER; i++) used_root->u.data.children[i] = NULL;
    for (int i = 0; i < CLOD_BTREE_ORDER - 1; i++) { used_root->u.keys[i] = 0; used_root->u.data.values[i] = 0; }

    // Set up free tree - uses special allocator that doesn't recurse
    impl->free_tree.root = free_root;
    impl->free_tree.count = 0;
    impl->free_tree.allocator = &tree_node_allocator;
    impl->free_tree.self_funding = false;
    impl->free_tree.alloc_cost = 0;

    // Set up used tree - also uses the special allocator
    impl->used_tree.root = used_root;
    impl->used_tree.count = 0;
    impl->used_tree.allocator = &tree_node_allocator;
    impl->used_tree.self_funding = false;
    impl->used_tree.alloc_cost = 0;

    // Initialize free tree with the initial free region
    size_t initial_free_blocks = blocks_for_size(backing_size - header_total);
    uint64_t data_addr = (uint64_t)data_start;
    
    uint64_t *val_ptr = clod_btree_insert(&impl->free_tree, data_addr);
    if (!val_ptr) {
        clod_vm_free(backing, backing_size);
        return 1;
    }
    *val_ptr = initial_free_blocks;

    *allocator_out = &impl->base;
    return 0;
}

static void *allocate_fn(clod_allocator *self, size_t size) {
    struct clod_allocator_impl *impl = (struct clod_allocator_impl *)self;
    return do_allocate(impl, size);
}

static void free_fn(clod_allocator *self, void *ptr) {
    if (!ptr || ptr == (void *)1) return;

    struct clod_allocator_impl *impl = (struct clod_allocator_impl *)self;
    uint64_t addr = (uint64_t)ptr;
    uint64_t value = 0;

    if (!clod_btree_get(&impl->used_tree, addr, &value)) return;

    int tag = DECODE_TAG(value);
    size_t size = DECODE_VALUE(value);

    clod_btree_delete(&impl->used_tree, addr, NULL);

    if (tag == TAG_CONTIGUOUS) {
        uint64_t blocks = blocks_for_size(size);
        // Temporarily disable coalesce to debug
        // coalesce_free(impl, addr, blocks);
        uint64_t *val_ptr = clod_btree_insert(&impl->free_tree, addr);
        if (val_ptr) *val_ptr = blocks;
    } else if (tag == TAG_DEDICATED) {
        impl->vm_free(impl->vm_user, ptr, size);
    }
}

void clod_allocator_destroy(clod_allocator *allocator) {
    struct clod_allocator_impl *impl = (struct clod_allocator_impl *)allocator;

    // Free all dedicated VM regions from used tree
    struct clod_btree_iter iter = CLOD_BTREE_ITER_INIT;
    uint64_t k, v;

    while (clod_btree_iter_next(&impl->used_tree, &iter, &k, &v)) {
        if (DECODE_TAG(v) == TAG_DEDICATED) {
            impl->vm_free(impl->vm_user, (void *)k, DECODE_VALUE(v));
        }
    }
    clod_btree_destroy(&impl->used_tree);

    // Free all regions from free tree
    iter = CLOD_BTREE_ITER_INIT;
    while (clod_btree_iter_next(&impl->free_tree, &iter, &k, &v)) {
        impl->vm_free(impl->vm_user, (void *)k, (size_t)(v * MIN_BLOCK_SIZE));
    }
    clod_btree_destroy(&impl->free_tree);

    clod_vm_free(impl->backing_start, impl->backing_size);
}
