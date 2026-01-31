#include "region_header.h"
#include <clod/nbt.h>
#include <clod/vmath.h>
#include <string.h>
#include <assert.h>

static int header_version(const char *file, const char *file_end) {
	assert(file_end > file);

	if (file_end - file >= HEADER_LIBCLOD_SIZE_MIN) {
		if (strncmp(file, HEADER_MAGIC, strlen(HEADER_MAGIC)) == 0) {
			return HEADER_VERSION_LIBCLOD;
		}
	}

	if (file_end - file >= HEADER_VANILLA_SIZE + HEADER_LIBCLOD_SIZE_MIN) {
		if (strncmp(file + HEADER_VANILLA_SIZE, HEADER_MAGIC, strlen(HEADER_MAGIC)) == 0) {
			return HEADER_VERSION_COMPOUND;
		}
	}

	if (file_end - file >= HEADER_VANILLA_SIZE) {
		return HEADER_VERSION_VANILLA;
	}

	return -1;
}

static struct {
	uint32_t nbt_checksum;
	uint32_t nbt_size;
	uint32_t generation;
} read_libclod_header(char *libclod_header, const char *file_end) {
	assert(file_end - libclod_header >= HEADER_LIBCLOD_SIZE_MIN);
	assert(strncmp(libclod_header, HEADER_MAGIC, strlen(HEADER_MAGIC)) == 0);

}

struct format_chunk_get {
	size_t chunk_offset;
	size_t chunk_size;
} format_chunk_get(char *file, const char *const file_end, int64_t *pos, uint8_t dims) {
	size_t chunk_index = vec_group(pos, dims, 10);

	switch (header_version(file, file_end)) {
		case HEADER_VERSION_VANILLA: {
			return (struct format_chunk_get){
				.chunk_offset = (size_t)beu24_dec(file + chunk_index * 4) * HEADER_VANILLA_SECTOR_SIZE,
				.chunk_size = (size_t)beu8_dec(file + chunk_index * 4 + 3) * HEADER_VANILLA_SECTOR_SIZE
			};
		}
		case HEADER_VERSION_COMPOUND: {
			return (struct format_chunk_get){
				.chunk_offset = (size_t)beu24_dec(file + chunk_index * 4) * HEADER_VANILLA_SECTOR_SIZE,
				.chunk_size = (size_t)beu8_dec(file + chunk_index * 4 + 3) * HEADER_VANILLA_SECTOR_SIZE
			};
		}
		case HEADER_VERSION_LIBCLOD: {
			char *nbt = get_nbt_root();
		}
	}


}

bool format_chunk_set(struct format *fmt, int64_t *pos) {
	return false;
}

