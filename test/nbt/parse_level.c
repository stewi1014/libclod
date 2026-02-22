#include <stdlib.h>

#include "test.h"
#include <clod/nbt.h>

const char parse_level_data[] = {
#embed "level.nbt"
};

int nbt_parse_level() {
	const size_t res = clod_nbt_tag_size((void*)parse_level_data, parse_level_data + sizeof(parse_level_data));
	check("correct NBT size", res == sizeof(parse_level_data));
	return 0;
}
