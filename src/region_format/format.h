#ifndef LIBCLOD_FORMAT_H
#define LIBCLOD_FORMAT_H

#include <clod/region_format.h>
#include <clod/table.h>
#include <stdint.h>

#define LIBCLOD_MAGIC \
	"\n\n"\
	"libclod extended region file format version 1.\n"\
	"See github.com/stewi1014/clod for format details.\n"\
	"\n\n"

constexpr size_t MAGIC_LEN = sizeof(LIBCLOD_MAGIC);

struct clod_rfmt {
	struct clod_rfmt_opts opts;

	uint8_t *data;
	size_t data_size;

#ifndef NDEBUG
	struct clod_table *held_locks;
#endif
};

constexpr size_t HEADER_SIZE = 20480u;

#define TABLE_SIZE 4192u
#define TABLE_ELEMS 1024u

#define SHADOW_TABLE_OFF(index) ((index) * 4u)
#define MTIME_TABLE_OFF(index) (4192u + (index) * 4u)
#define MAGIC_OFF 8192u
#define MAGIC_SIZE MAGIC_LEN
#define CHUNK_PREFIX_OFF 8320u
#define CHUNK_PREFIX_SIZE 30u
#define CHUNK_EXTENSION_OFF 8350u
#define CHUNK_EXTENSION_SIZE 10u
#define SECTOR_SIZE_OFF 8360u
#define FILE_LOCK_OFF 8364u
#define CHECKPOINT_CRC_OFF 8372u
#define SHADOWED_COUNT_OFF 8376u
#define LOCK_TABLE_OFF(index) (12288u + (index) * 4u)
#define CHECKPOINT_TABLE_OFF(index) (16384u + (index) * 4u)

typedef struct {
	uint32_t offset : 24;
	uint8_t size : 8;
} chunk_loc;
static inline chunk_loc chunk_loc_dec(uint8_t ptr[4]) {
	chunk_loc loc;
	loc.offset = (uint32_t)(ptr[0] << 16 | ptr[1] << 8 | ptr[2]);
	loc.size = ptr[3];
	return loc;
}
static inline void chunk_loc_enc(uint8_t ptr[4], chunk_loc loc) {
	ptr[0] = (uint8_t)(loc.offset >> 16);
	ptr[1] = (uint8_t)(loc.offset >> 8);
	ptr[2] = (uint8_t)loc.offset;
	ptr[3] = loc.size;
}

#endif
