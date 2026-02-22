#include <stdlib.h>

#include "test.h"
#include <clod/nbt.h>

const uint8_t print_level_data[] = {
#embed "level.nbt"
};

void print_recursive(const uint8_t *payload, void *end, const char type, const int indent) {
	if (type == CLOD_NBT_COMPOUND || type == CLOD_NBT_LIST) {
		struct clod_nbt_iter iter = CLOD_NBT_ITER_ZERO;
		while (clod_nbt_iter_next(payload, end, type, &iter)) {
			for (int i = 0; i < indent; i++) printf("\t");
			if (type == CLOD_NBT_LIST) {
				printf("[%d]\n", iter.index);
				print_recursive(iter.payload, end, iter.type, indent + 1);
			} else {
				const clod_sstr name = clod_nbt_tag_name(iter.tag, end);
				printf("%.*s\n", (int)name.size, name.ptr);
				print_recursive(iter.payload, end, iter.type, indent + 1);
			}
		}
	}
}

int nbt_print_level() {
	const size_t res = clod_nbt_tag_size((void*)print_level_data, print_level_data + sizeof(print_level_data));
	check("correct NBT size", res == sizeof(print_level_data));

	print_recursive(print_level_data, (void*)(print_level_data + sizeof(print_level_data)), CLOD_NBT_COMPOUND, 0);
	return 0;
}
