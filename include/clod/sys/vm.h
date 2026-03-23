/**
 * @file clod/sys/vm.h
 * @ingroup sys System Methods
 *
 * Methods relating to system virtual memory.
 * For example, clod/memory.h uses this as a backend for allocation.
 */
#ifndef LIBCLOD_VM_H
#define LIBCLOD_VM_H

#include <clod/lib.h>

/// Get the system page size.
/// Currently set to 4096 constant as other page sizes are rare,
/// and linux offers no way to query it.
size_t clod_vm_page_size();

/// Allocate virtual memory.
void *clod_vm_alloc(size_t size);

/// Free virtual memory.
/// Size must be equal to the value passed to clod_vm_alloc.
void clod_vm_free(void *ptr, size_t size);

#endif
