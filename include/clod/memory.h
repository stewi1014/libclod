/**
 * @file clod/memory.h
 * @brief Memory allocation methods.
 */
#ifndef LIBCLOD_MEMORY_H
#define LIBCLOD_MEMORY_H

#include <clod/lib.h>

typedef struct clod_allocator clod_allocator;

/**
 * Memory allocator methods.
 * Libclod provides a way to create an allocator built using system virtual memory.
 * Methods that take an allocator for internal usage do not assume the implementation is libclod's.
 * As such, custom allocations methods can be provided everywhere clod_allocator is used unless stated otherwise.
 */
struct clod_allocator {
	/**
	 * Allocate memory.
	 * @param[in] self Implementation defined value.
	 * @param[in] size Size in bytes of the memory to allocate.
	 * @return Pointer to the newly allocated memory.
	 */
	void *(*allocate)(clod_allocator *self, size_t size);

	/**
	 * Free memory.
	 * @param[in] ptr Pointer previously returned from \p allocate to free.
	 * @param[in] self Implementation defined value.
	 */
	void (*free)(clod_allocator *self, void *ptr);
};

/// Configuration options for libclod's memory allocator.
struct clod_allocator_opts {
	/// Size of this struct for future-proofing.
	size_t allocator_opts_size;

	/// Page size. Backing memory is only allocated with multiples of this.
	/// Must be a multiple of 2.
	/// Defaults to the result of clod_vm_page_size.
	size_t page_size;

	/// The initial size of the backing memory in pages.
	/// The actual amount of memory that can be allocated will be less than this, and no method
	/// for discerning the actual amount of memory that can be allocated exists. This is because
	/// fragmentation can and will happen through usage, and it can waste arbitrary amounts of memory.
	/// Defaults to the system memory size, or 16GiB if not available.
	size_t initial_pages;

	/// Allocations above this size will be fulfilled with a dedicated call to \p vm_alloc.
	/// Defaults to 64MiB.
	size_t dedicated_vm_threshold;

	/// If true, the allocator will only allocate a single backing memory region of
	/// size \p initial_pages, and allocations which do not fit in the memory region will fail
	/// instead of allocating more backing storage.
	/// Defaults to false.
	bool no_grow;

	/// Passed to the allocator's vm_alloc method.
	void *vm_user;

	/// Method used to get backing memory for the allocator i.e. virtual-memory mapping function.
	/// It is only called with multiples of \p page_size.
	/// Defaults to clod_vm_alloc.
	void *(*vm_alloc)(void *user, size_t size);

	/// Method used to free backing memory for the allocator i.e. virtual-memory unmapping function.
	/// Defaults to clod_vm_free.
	void (*vm_free)(void *user, void *ptr, size_t size);
};

/**
 * Create a memory allocator.
 *
 * It is not thread safe. Instead, make a dedicated allocator for each thread, and if you need to
 * free memory in a different thread from where it was allocated - fix your program architecture.
 *
 * Allocations are aligned to either the power of two equal to or greater than \p size
 * or the configured page size; whichever is smaller.
 *
 * The contents of allocated memory is undefined.
 *
 * Allocating a size of zero returns a non-null pointer that can be freed.
 * Freeing nullptr does nothing.
 *
 * @param[in] opts (nullable) Allocator configuration options.
 * @return New allocator, or nullptr on allocation failure or invalid options.
 */
CLOD_API
clod_allocator *clod_allocator_create(const struct clod_allocator_opts *opts);

/**
 * Free a whole memory allocator that was created with clod_allocator_create.
 * All memory allocated with the allocator is also freed.
 * @param[in] allocator Memory allocator to free.
 */
CLOD_API CLOD_NONNULL(1)
void clod_allocator_destroy(clod_allocator *allocator);

#endif
