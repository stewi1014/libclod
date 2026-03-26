#include <stdlib.h>

#include "debug.h"
#include <clod/structures/nbt.h>
#include <clod/string.h>

const uint8_t print_level_data[] = {
#embed "level.nbt"
};

void print_recursive(const uint8_t *payload, void *end, const uint8_t type, const int indent) {
	if (type == CLOD_NBT_COMPOUND || type == CLOD_NBT_LIST) {
		struct clod_nbt_iter iter = CLOD_NBT_ITER_ZERO;
		while (clod_nbt_iter_next(payload, end, type, &iter)) {
			for (int i = 0; i < indent; i++) debug(CLOD_TEST, "\t");
			if (type == CLOD_NBT_LIST) {
				debug(CLOD_TEST, "[%i]\n", iter.index);
				print_recursive(iter.payload, end, iter.type, indent + 1);
			} else {
				const struct clod_string name = clod_nbt_tag_name(iter.tag, end);
				debug(CLOD_TEST, "%.*s\n", (int)name.len, name.ptr);
				print_recursive(iter.payload, end, iter.type, indent + 1);
			}
		}
	}
}

int structures_nbt_print_level(int, char[]) {
	const size_t res = clod_nbt_tag_size((void*)print_level_data, print_level_data + sizeof(print_level_data));
	assert_fatal(CLOD_TEST, res == sizeof(print_level_data), "correct NBT size");

	print_recursive(print_level_data, (void*)(print_level_data + sizeof(print_level_data)), CLOD_NBT_COMPOUND, 0);
	return 0;
}
