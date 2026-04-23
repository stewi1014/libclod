#ifndef LIBCLOD_MEMORY_TREE_H
#define LIBCLOD_MEMORY_TREE_H

#include <stdint.h>
#include <stddef.h>

#define TYPE_INTL 1
#define TYPE_LEAF 2

#define INTL_CAP 63
#define LEAF_CAP 14

#define VALUE_SIZE 64

struct node_common {
	uint32_t checksum;

	unsigned char type;

	unsigned char len;
};

struct node_intl {
	struct node_common common;

	uintptr_t keys[INTL_CAP];
	union node *branches[INTL_CAP + 1];
};

struct node_leaf {
	struct node_common common;

	uintptr_t keys[LEAF_CAP];
	char values[VALUE_SIZE][LEAF_CAP];
};

union node {
	struct node_common common;
	struct node_intl intl;
	struct node_leaf leaf;
};

static_assert(sizeof(struct node_intl) >= sizeof(union node) * 98 / 100);
static_assert(sizeof(struct node_leaf) >= sizeof(union node) * 98 / 100);

#endif
