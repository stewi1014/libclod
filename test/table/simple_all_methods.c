#include "debug.h"
#include <string.h>
#include <../../include/clod/table.h>

#define STR "abcd"
#define LEN strlen(STR)

int table_simple_all_methods(int, char[]) {
	const char *val = STR;
	const char *val2 = STR;
	struct clod_table_iter iter;

	void *existing;

	struct clod_table *t = clod_table_create(nullptr);
	assert_fatal(CLOD_TEST, clod_table_len(t) == 0, "Correct length.");
	assert_fatal(CLOD_TEST, clod_table_get(t, val, strlen("abcd")) == nullptr,);
	assert_fatal(CLOD_TEST, clod_table_del(t, val, strlen("abcd")) == nullptr, "");
	assert_fatal(CLOD_TEST, clod_table_add(t, val2, strlen("abcd"), &existing) && existing == nullptr, "");
	assert_fatal(CLOD_TEST, !clod_table_add(t, val, strlen("abcd"), &existing) && existing == val2, "");
	assert_fatal(CLOD_TEST, clod_table_set(t, val, strlen("abcd"), &existing) && existing == val2, "");
	iter = CLOD_TABLE_ITER_INIT;
	assert_fatal(CLOD_TEST, clod_table_iter(t, &iter), "");
	assert_fatal(CLOD_TEST, iter.element == val, "");
	assert_fatal(CLOD_TEST, iter.key_size == strlen("abcd"), "");
	assert_fatal(CLOD_TEST, !clod_table_iter(t, &iter), "");
	assert_fatal(CLOD_TEST, clod_table_get(t, "abcd", strlen("abcd")) == val, "");
	assert_fatal(CLOD_TEST, clod_table_del(t, "abcd", strlen("abcd")) == val, "");
	assert_fatal(CLOD_TEST, clod_table_get(t, "abcd", strlen("abcd")) == nullptr, "");
	assert_fatal(CLOD_TEST, clod_table_del(t, "abcd", strlen("abcd")) == nullptr, "");
	clod_table_destroy(t);

	return 0;
}
