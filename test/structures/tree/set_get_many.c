#include "config.h"
#include "debug.h"
#include <clod/structures/tree.h>
#include "clod/sys/vm.h"

#define MEMORY_SIZE (4096 * 512)

int structures_tree_set_get_many_impl(struct clod_tree *tree, int NUM_ELEMS) {
	tree->key_size = sizeof(int);
	tree->val_size = sizeof(int);

	assert_fatal(CLOD_TEST, clod_tree_create(tree), "Failed to create tree");
	for (int i = 0; i < NUM_ELEMS; i++) {

		int v = i ^ (int)0xAAAAAAAA;
		assert_fatal(CLOD_TEST, clod_tree_add(tree, &i, &v, nullptr, nullptr), "Inserting non-existent key should succeed.");

		assert_fatal(CLOD_TEST, clod_tree_validate(tree), "Tree must be valid. Adding value %i.", i);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {

		int v = i ^ (int)0xAAAAAAAA;
		int r_k, r_v;
		const bool res = clod_tree_get(tree, &i, &r_k, &r_v);

		assert_fatal(CLOD_TEST, res == true, "Fetching existing key %i should succeed.", i);
		assert_fatal(CLOD_TEST, r_k == i, "clod_tree_get returned key %i when trying to get key %i.", r_k, i);
		assert_fatal(CLOD_TEST, r_v == v, "clod_tree_get returned value %i when trying to get key %i. Wanted %i.", r_v, i, v);

		assert_fatal(CLOD_TEST, clod_tree_validate(tree), "Tree must be valid. Fetching value %i.", i);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {

		int v = i ^ 0x55555555;
		int r_k, r_v;
		const bool res = clod_tree_add(tree, &i, &v, &r_k, &r_v);

		assert_fatal(CLOD_TEST, res == false, "Inserting existing key %i should fail but instead it succeeded.", i);

		assert_fatal(CLOD_TEST, r_k == i, "clod_tree_add returned key %i when trying to add preexisting key. Wanted %i.", r_k, i);
		assert_fatal(CLOD_TEST, r_v == (i ^ (int)0xAAAAAAAA), "clod_tree_add returned value %i when trying to add preexisting key %i with value %i.", r_v, i, (i ^ (int)0xAAAAAAAA));

		assert_fatal(CLOD_TEST, clod_tree_validate(tree), "Tree must be valid. Adding existing value %i.", i);
	}
	for (int i = 0; i < NUM_ELEMS; i += 2) {
		int r_k, r_v;
		assert_fatal(CLOD_TEST, clod_tree_del(tree, &i, &r_k, &r_v) == true, "Should successfully delete preexisting key %i.", i);
		assert_fatal(CLOD_TEST, r_k == i, "clod_tree_del returned key %i, but wanted %i.", r_k, i);
		assert_fatal(CLOD_TEST, r_v == (i ^ (int)0xAAAAAAAA), "clod_tree_del returned value %i, but wanted %i for key %i.", r_v, (i ^ (int)0xAAAAAAAA), i);

		assert_fatal(CLOD_TEST, clod_tree_del(tree, &i, &r_k, &r_v) == false, "Should fail to delete nonexistent key.");

		assert_fatal(CLOD_TEST, clod_tree_validate(tree), "Tree must be valid. Removing value %i.", i);
	}
	for (int i = 1; i < NUM_ELEMS; i += 2) {

		//print_tree(tree);
		int r_k, r_v;
		assert_fatal(CLOD_TEST, clod_tree_get(tree, &i, &r_k, &r_v) == true, "Should successfully get existing key %i.", i);
		assert_fatal(CLOD_TEST, r_k == i, "clod_tree_get returned key %i, but wanted %i.", r_k, i);
		assert_fatal(CLOD_TEST, r_v == (i ^ (int)0xAAAAAAAA), "clod_tree_get returned value %i, but wanted %i for key %i.", r_v, (i ^ (int)0xAAAAAAAA), i);

		assert_fatal(CLOD_TEST, clod_tree_validate(tree), "Tree must be valid. Removing value %i.", i);
	}

	clod_tree_destroy(tree);

	return 0;
}

struct dumb_allocator {
	void *memory;
	size_t offset;
	size_t dead_space;
	size_t freed;
	size_t node_size;
};

void *structures_tree_set_get_many_alloc(const size_t size, void *self) {
	struct dumb_allocator *alloc = self;
	assert_fatal(CLOD_TEST, alloc->offset + size + alloc->dead_space <= MEMORY_SIZE, "Out of space for test tree.");

	assert_fatal(CLOD_TEST, size == alloc->node_size,
		"Tree must only allocate memory with the same size as its node. %i, but %i was requested.",
		alloc->node_size, size);

	void *ptr = (char*)alloc->memory + alloc->offset;
	alloc->offset += alloc->node_size + alloc->dead_space;
	return ptr;
}

void structures_tree_set_get_many_free(void *ptr, void *self) {
	struct dumb_allocator *alloc = self;
	alloc->freed += alloc->node_size + alloc->dead_space;

	for (unsigned i = 0; i < alloc->node_size; i++) {
		((char*)ptr)[i] = 0;
	}
}

int structures_tree_set_get_many_cmp(const void *key1, const void *key2, void *) {
	return *(int*)key1 - *(int*)key2;
}

int structures_tree_set_get_many(int, char[]) {
	struct dumb_allocator alloc;

	alloc.memory = clod_vm_alloc(MEMORY_SIZE);

	struct clod_tree tree = {0};
	tree.compare = &structures_tree_set_get_many_cmp;
	tree.allocator = (clod_allocator){
		.self = &alloc,
		.allocate = &structures_tree_set_get_many_alloc,
		.free = &structures_tree_set_get_many_free
	};

	for (int NUM_ELEMS = 10; NUM_ELEMS < 5000; NUM_ELEMS *= 7) {
		for (size_t NODE_SIZE = 56; NODE_SIZE < (1<<14); NODE_SIZE *= 3) {
			alloc.offset = 0;
			alloc.dead_space = 8;
			alloc.freed = 0;
			alloc.node_size = (unsigned short)NODE_SIZE;
			tree.node_size = (unsigned short)NODE_SIZE;
			for (int j = 0; j + (int)NODE_SIZE + 8 < MEMORY_SIZE; j += (int)NODE_SIZE + 8) {
				uint64_t *canary = (uint64_t*)((char*)alloc.memory + j + NODE_SIZE);
				*canary = 0xAAAAAAAAAAAAAAA;
			}
			structures_tree_set_get_many_impl(&tree, NUM_ELEMS);
			for (int j = 0; j + (int)NODE_SIZE + 8 < MEMORY_SIZE; j += (int)NODE_SIZE + 8) {
				const uint64_t *canary = (uint64_t*)((char*)alloc.memory + j + NODE_SIZE);
				assert_fatal(CLOD_TEST, *canary == 0xAAAAAAAAAAAAAAA, "Canary should not be modified.");
			}

			assert_fatal(CLOD_TEST, alloc.offset == alloc.freed, "All allocated memory should be freed on tree destroy.");
		}
	}

	clod_vm_free(alloc.memory, MEMORY_SIZE);

	return 0;
}