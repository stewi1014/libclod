#include <stdlib.h>

#include "test.h"
#include <clod/structures/nbt.h>

const unsigned char parse_level_data[] = {
#embed "level.nbt"
};

int structures_nbt_parse_level(int, char[]) {
	const size_t res = clod_nbt_tag_size((void*)parse_level_data, parse_level_data + sizeof(parse_level_data));
	check("correct NBT size", res == sizeof(parse_level_data));
	return 0;
}
