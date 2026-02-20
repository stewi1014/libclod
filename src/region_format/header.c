#include "format.h"
#include "endian_big.h"
#include <string.h>

#define TABLE_SIZE 4192u
#define TABLE_ELEMS 1024u

#define SHADOW_TABLE_OFF(index) ((index) * 4u)
#define SHADOW_TABLE_SIZE TABLE_SIZE
#define MTIME_TABLE_OFF(index) (4192u + (index) * 4u)
#define MTIME_TABLE_SIZE TABLE_SIZE
#define MAGIC_OFF 8192u
#define MAGIC_SIZE 128u
#define CHUNK_PREFIX_OFF 8320u
#define CHUNK_PREFIX_SIZE 31u
#define CHUNK_EXTENSION_OFF 8351u
#define CHUNK_EXTENSION_SIZE 11u
#define SECTOR_SIZE_OFF 8364u
#define FILE_LOCK_OFF 8368u
#define FILE_SIZE_OFF 8372u
#define CHECKPOINT_CRC_OFF 8376u
#define SHADOWED_SECTORS_OFF 8380u
#define LOCK_TABLE_OFF(index) (12288u + (index) * 4u)
#define LOCK_TABLE_SIZE TABLE_SIZE
#define CHECKPOINT_TABLE_OFF(index) (16384u + (index) * 4u)
#define CHECKPOINT_TABLE_SIZE TABLE_SIZE

void magic_set(struct clod_rfmt *rfmt, const char *magic) {
	strncpy((char*)rfmt->data + MAGIC_OFF, magic, MAGIC_SIZE);
	((char*)rfmt->data)[MAGIC_OFF + MAGIC_SIZE - 1] = '\0';
}

void magic_get(struct clod_rfmt *rfmt, char magic[MAGIC_SIZE]) {
	strncpy(magic, (char*)rfmt->data + MAGIC_OFF, MAGIC_SIZE);
	magic[MAGIC_SIZE - 1] = '\0';
}

void chunk_prefix_set(struct clod_rfmt *rfmt, const char *prefix) {
	strncpy((char*)rfmt->data + CHUNK_PREFIX_OFF, prefix, CHUNK_PREFIX_SIZE);
	((char*)rfmt->data)[CHUNK_PREFIX_OFF + CHUNK_PREFIX_SIZE - 1] = '\0';
}

void chunk_prefix_get(const struct clod_rfmt *rfmt, char prefix[CHUNK_PREFIX_SIZE]) {
	strncpy(prefix, (char*)rfmt->data + CHUNK_PREFIX_OFF, CHUNK_PREFIX_SIZE);
	prefix[CHUNK_PREFIX_SIZE - 1] = '\0';
}

void chunk_extension_set(struct clod_rfmt *rfmt, const char *extension) {
	strncpy((char*)rfmt->data + CHUNK_EXTENSION_OFF, extension, CHUNK_EXTENSION_SIZE);
	((char*)rfmt->data)[CHUNK_EXTENSION_OFF + CHUNK_EXTENSION_SIZE - 1] = '\0';
}

void chunk_extension_get(const struct clod_rfmt *rfmt, char extension[CHUNK_EXTENSION_SIZE]) {
	strncpy(extension, (char*)rfmt->data + CHUNK_EXTENSION_OFF, CHUNK_EXTENSION_SIZE);
	extension[CHUNK_EXTENSION_SIZE - 1] = '\0';
}

void sector_size_set(struct clod_rfmt *rfmt, uint32_t sector_size) {
	beu32_enc(rfmt->data + SECTOR_SIZE_OFF, sector_size);
}

uint32_t sector_size_get(const struct clod_rfmt *rfmt) {
	return beu32_dec(rfmt->data + SECTOR_SIZE_OFF);
}


