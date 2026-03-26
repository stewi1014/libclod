#include <stdlib.h>

#include "debug.h"
#include <clod/structures/nbt.h>

const unsigned char parse_level_data[] = {
#embed "level.nbt"
};

int structures_nbt_parse_level(int, char[]) {
	const size_t res = clod_nbt_tag_size((void*)parse_level_data, parse_level_data + sizeof(parse_level_data));
	assert_fatal(CLOD_TEST, res == sizeof(parse_level_data), "correct NBT size");
	return 0;
}
