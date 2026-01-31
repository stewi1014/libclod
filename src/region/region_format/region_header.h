#ifndef CLOD_REGION_FORMAT_HEADER_H
#define CLOD_REGION_FORMAT_HEADER_H

#include <clod/big_endian.h>
#include <clod/region.h>

#define HEADER_VERSION_VANILLA 1
#define HEADER_VERSION_LIBCLOD 2
#define HEADER_VERSION_COMPOUND 3

struct format {
	char *file_data;
	size_t file_size;

	int version;
};

#define FORMAT_NULL ((struct format){ .file_data = nullptr })

#define HEADER_MAGIC_SIZE 128
#define HEADER_MAGIC \
	"\n\n"\
	"libclod extended region file format version 1.\n"\
	"See github.com/stewi1014/clod for format details.\n"\
	"\n\n"

static_assert(sizeof(HEADER_MAGIC) <= HEADER_MAGIC_SIZE);

#define HEADER_VANILLA_SIZE 8192
#define HEADER_LIBCLOD_SIZE_MIN (HEADER_MAGIC_SIZE + 128)

#define HEADER_VANILLA_SECTOR_SIZE 4096

#endif
