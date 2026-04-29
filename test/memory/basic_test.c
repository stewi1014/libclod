#include <clod/memory.h>

#include "debug.h"

#define TEST_DATA "abcdefggn02v469mmfd2,8n6f52n4-962f-m6458"
#define TEST_ALLOCATIONS 100000

int memory_basic_test() {
	clod_allocator *allocator = clod_allocator_create(nullptr);
	assert_fatal(CLOD_TEST, allocator, "Creating allocator failed.");

	char *allocated[TEST_ALLOCATIONS];
	for (size_t i = 0; i < TEST_ALLOCATIONS; i++) {
		const size_t size = i % sizeof(TEST_DATA);

		allocated[i] = allocator->allocate(allocator, size);

		for (size_t j = 0; j < size; j++) {
			allocated[i][j] = TEST_DATA[j] + (char)i;
		}
	}

	for (size_t i = 0; i < TEST_ALLOCATIONS; i++) {
		const size_t size = i % sizeof(TEST_DATA);

		for (size_t j = 0; j < size; j++) {
			assert_fatal(CLOD_TEST, allocated[i][j] == (char)(TEST_DATA[j] + (char)i), "Data in allocated memory of size %size is consistent.", size);
		}

		allocator->free(allocator, allocated[i]);
	}

	clod_allocator_destroy(allocator);
	return 0;
}
