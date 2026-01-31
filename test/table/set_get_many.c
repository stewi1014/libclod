#include "../test.h"
#include <clod/table.h>
#include <stdlib.h>
#include <string.h>

#define NUM_ELEMS 100000

int main() {
	int elems[NUM_ELEMS];
	int elems2[NUM_ELEMS];
	for (int i = 0; i < NUM_ELEMS; i++) {
		elems[i] = i;
		elems2[i] = NUM_ELEMS + i;
	}

	struct clod_table *t = clod_table_create(nullptr);
	for (int i = 0; i < NUM_ELEMS; i++) {
		check("no element is returned when adding a unique key", clod_table_add(t, &elems[i], 4) == nullptr);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check("existing element is returned when trying to add an existing key", clod_table_add(t, &elems[i], 4) == &elems[i]);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		const int *p = &i;
		check("correct existing element is returned when replacing an existing key", clod_table_set(t, p, 4) == &elems[i]);
		check("correct updated existing element is returned when replacing an existing key", clod_table_set(t, &elems[i], 4) == p);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check("adding more keys succeeds", clod_table_add(t, &elems2[i], 4) == nullptr);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check("old keys still exist after modifications", clod_table_add(t, &elems[i], 4) == &elems[i]);
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

	return EXIT_SUCCESS;
}
