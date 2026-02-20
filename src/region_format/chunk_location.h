#ifndef LIBCLOD_CHUNK_LOCATION_H
#define LIBCLOD_CHUNK_LOCATION_H

#include <stdint.h>

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
