#include <clod/memory.h>
#include <clod/sys/vm.h>

#include "allocator.h"

clod_allocator *clod_allocator_create(const struct clod_allocator_opts *opts) {
	if (opts == nullptr) {
		opts = &(struct clod_allocator_opts){0};
	} else if (opts->allocator_opts_size < sizeof(*opts)) {
		return nullptr;
	}

}

void *clod_alloc(clod_allocator *allocator, const size_t size) {
	if (size == 0) {
		return (char*)allocator + allocator->size;
	}


}

void clod_free(clod_allocator *allocator, void *ptr) {
	if (ptr == nullptr) return;
	if (ptr == (char*)allocator + allocator->size) return;


}

void clod_allocator_destroy(clod_allocator *allocator) {

}
