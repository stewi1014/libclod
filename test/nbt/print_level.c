#include <stdlib.h>

#include "test.h"
#include <clod/nbt.h>

const char level_data[] = {
#embed "level.nbt"
};

void print_recursive(const char *payload, void *end, const char type, const int indent) {
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

int main() {
	const size_t res = clod_nbt_tag_size((void*)level_data, level_data + sizeof(level_data));
	check("correct NBT size", res == sizeof(level_data));

	print_recursive(level_data, (void*)(level_data + sizeof(level_data)), CLOD_NBT_COMPOUND, 0);
}
