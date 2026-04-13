#include "debug.h"
#include <../../include/clod/table.h>
#include <string.h>

#define NUM_ELEMS 100000

// I can't be fucked with updating all the tests to use assert_fatal right now.
#define check(a, b) assert_fatal(CLOD_TEST, b, a)

int table_set_get_many(int, char[]) {
	int elems[NUM_ELEMS];
	int elems2[NUM_ELEMS];
	for (int i = 0; i < NUM_ELEMS; i++) {
		elems[i] = i;
		elems2[i] = NUM_ELEMS + i;
	}

	struct clod_table *t = clod_table_create(nullptr);
	for (int i = 0; i < NUM_ELEMS; i++) {
		void *existing;


		check("success when adding a unique key", clod_table_add(t, &elems[i], 4, &existing));
		check("no element was returned when adding a unique key", existing == nullptr);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		void *existing;
		check("adding duplicate key fails", !clod_table_add(t, &elems[i], 4, &existing));
		check("returned existing element is correct", existing == &elems[i]);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		const int *p = &i;
		void *existing;
		check("replacing an existing key works", clod_table_set(t, p, 4, &existing));
		check("replacing returns correct existing element", existing == &elems[i]);
		check("replacing an existing key a second time works", clod_table_set(t, &elems[i], 4, &existing));
		check("replacing an existing key a second time returns the correct existing element", existing == p);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check("adding keys without specifying an existing out works", clod_table_add(t, &elems2[i], 4, nullptr));
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check("getting old keys after adding some more returns correct elements", clod_table_get(t, &elems[i], 4) == &elems[i]);
	}
	bool found[NUM_ELEMS * 2] = {0};
	struct clod_table_iter iter = {0};
	while (clod_table_iter(t, &iter)) {
		found[*(int*)iter.element] = true;
	}
	for (int i = 0; i < NUM_ELEMS * 2; i++) {
		check("iterating found all keys", found[i]);
	}

	for (int i = 0; i < NUM_ELEMS - 1; i++) {
		check("deleting keys returns expected element", clod_table_del(t, &i, 4) == &elems[i]);
	}

	memset(found, 0, sizeof(found));
	while (clod_table_iter(t, &iter)) {
		found[*(int*)iter.element] = true;
	}
	for (int i = NUM_ELEMS; i < NUM_ELEMS * 2; i++) {
		check("iterating found all non-deleted keys", found[i]);
	}

	for (int i = 0; i < NUM_ELEMS; i++) {
		check("non-deleted keys still exist", clod_table_get(t, &elems2[i], 4) == &elems2[i]);
	}
	clod_table_destroy(t);

	return 0;
}
