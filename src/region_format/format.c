#include "format.h"
#include "clod_config.h"
#include <clod/region_format.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void *default_malloc(size_t size, void*) { return malloc(size); }
static void default_free(void *ptr, void*) { free(ptr); }
struct clod_rfmt *rfmt_new(struct clod_rfmt_opts *opts) {
	struct clod_rfmt *rfmt;
	if (opts->malloc_func)
		rfmt = opts->malloc_func(sizeof(*rfmt), opts->user);
	else
		rfmt = default_malloc(sizeof(*rfmt), opts->user);
	if (!rfmt) return nullptr;

	rfmt->opts = *opts;
	if (rfmt->opts.dead_lock_timeout_ms == 0) rfmt->opts.dead_lock_timeout_ms = 5000;
	if (!rfmt->opts.malloc_func) opts->malloc_func = default_malloc;
	if (!rfmt->opts.free_func) opts->free_func = default_free;
	rfmt->data = nullptr;
	rfmt->data_size = 0;

#if CLOD_REGION_DEBUG
	rfmt->file_locked = false;
	bitarray_unset_all(rfmt->held_locks);
	bitarray_unset_all(rfmt->observing_locks);
#endif

	return rfmt;
}
enum clod_rfmt_result clod_rfmt_free(struct clod_rfmt *rfmt) {
#if CLOD_REGION_DEBUG
	if (
		rfmt->file_locked ||
		!bitarray_all_unset(rfmt->held_locks) ||
		!bitarray_all_unset(rfmt->observing_locks)
	) {
		rfmt->opts.free_func(rfmt, rfmt->opts.user);
		return CLOD_RFMT_MISUSE;
	}
#endif
	rfmt->opts.free_func(rfmt, rfmt->opts.user);
	return CLOD_RFMT_OK;
}

enum clod_rfmt_result clod_rfmt_init_new(
	struct clod_rfmt **rfmt_out,
	struct clod_rfmt_opts *opts,
	char *chunk_filename_prefix,
	char *chunk_filename_extension,
	uint32_t sector_size
) {
	struct clod_rfmt *rfmt = rfmt_new(opts);
	if (!rfmt) return CLOD_RFMT_ALLOCATION_FAILURE;
	if (!rfmt->opts.file_manage(&rfmt->data, &rfmt->data_size, &HEADER_SIZE, rfmt->opts.user)) {
		clod_rfmt_free(rfmt);
		return CLOD_RFMT_FILE_MANAGE_ERROR;
	}

	if (!rfmt->data || rfmt->data_size < HEADER_SIZE) {
		clod_rfmt_free(rfmt);
		return CLOD_RFMT_FILE_MANAGE_ERROR;
	}

	memset(rfmt->data, 0, HEADER_SIZE);
	memset(rfmt->data + LIBCLOD_MAG)
}

enum clod_rfmt_result clod_rfmt_init_rw(struct clod_rfmt **rfmt_out, struct clod_rfmt_opts *opts) {
	struct clod_rfmt *rfmt = rfmt_new(opts);
	if (!rfmt) return nullptr;
}

enum clod_rfmt_result clod_rfmt_init_ro(struct clod_rfmt **rfmt_out, struct clod_rfmt_opts *opts) {
	struct clod_rfmt *rfmt = rfmt_new(opts);
	if (!rfmt) return nullptr;

}

/*
typedef struct {
	uint32_t offset: 24;
	uint8_t size: 8;
} chunk_loc;

static inline chunk_loc chunk_loc_dec(uint32_t loc) { return (chunk_loc){loc >> 8, loc & 0xFF}; }
static inline uint32_t chunk_loc_enc(chunk_loc loc) { return (uint32_t)(loc.offset << 8 | loc.size); }

static inline uint32_t sector_size(const uint8_t *data) { return beu32_dec(data + 8360); }
static inline uint32_t file_sectors_add(uint8_t *data, uint32_t delta) { return atomic_beu32_add(data + 8364 , delta); }
static inline void file_sectors_set(uint8_t *data, uint32_t val) { atomic_beu32_store(data + 8364, val); }
static inline uint32_t checkpoint_crc(const uint8_t *data) { return beu32_dec(data + 8372); }
static inline void checkpoint_crc_set(uint8_t *data, uint32_t val) { beu32_enc(data + 8372, val); }
static inline uint32_t shadowed_chunk_count(const uint8_t *data) { return atomic_beu32_load(data + 8376); }
static inline void shadowed_chunk_count_set(uint8_t *data, uint32_t val) { atomic_beu32_store(data + 8376, val); }
static inline uint32_t shadowed_chunk_count_add(uint8_t *data, uint32_t val) { return atomic_beu32_add(data + 8376, val); }
static inline chunk_loc shadowed(const uint8_t *data, int chunk_index) { return chunk_loc_dec(beu32_dec(data + chunk_index * 4)); }
static inline void shadowed_set(uint8_t *data, int chunk_index, chunk_loc loc) { beu32_enc(data + chunk_index * 4, chunk_loc_enc(loc)); }
static inline chunk_loc checkpoint_set(const uint8_t *data, int chunk_index) { return chunk_loc_dec(beu32_dec(data + 16384 + chunk_index * 4)); }
static inline void checkpoint(uint8_t *data, int chunk_index, chunk_loc loc) { beu32_enc(data + 16384 + chunk_index * 4, chunk_loc_enc(loc)); }
static inline uint32_t mtime(const uint8_t *data, int chunk_index) { return beu32_dec(data + 4096 + chunk_index * 4); }
static inline void mtime_set(uint8_t *data, int chunk_index, uint32_t mtime) { beu32_enc(data + 4096 + chunk_index * 4, mtime); }
static inline uint32_t generation(const uint8_t *data, int chunk_index) { return atomic_beu32_load(data + 12288 + chunk_index * 4); }
static inline void generation_set(uint8_t *data, int chunk_index, uint32_t generation) { atomic_beu32_store(data + 12288 + chunk_index * 4, generation); }
static inline uint32_t generation_cas(uint8_t *data, int chunk_index, uint32_t *expected, uint32_t desired) { return atomic_beu32_cas(data + 12288 + chunk_index * 4, expected, desired); }


static enum clod_rhdr_result lock_one(const uint8_t *data) {

}

static enum clod_rhdr_result lock_all(const uint8_t *data) {

}

enum clod_rhdr_type clod_rhdr_detect_type(const uint8_t *data, size_t data_size) {
	if (data_size >= HEADER_LIBCLOD_SIZE && memcmp(data + 8192, LIBCLOD_MAGIC, LIBCLOD_MAGIC_LEN) == 0) {
		// Appears to be a libclod header.
		// Make sure a vanilla implementation hasn't overwritten the libclod header.
		const uint32_t min_offset_sectors = ceil_div(HEADER_LIBCLOD_SIZE, sector_size(data));

		for (int i = 0; i < 1024; i++) {
			bool ok = true;
			observe_gen(data, i) {
				const chunk_loc loc = shadowed(data, i);
				if (loc.size == 0) ok = true;
				else if (loc.offset < min_offset_sectors) ok = false;
			}
			if (!ok) return CLOD_RHDR_VANILLA;
		}

		return CLOD_RHDR_LIBCLOD;
	}

	if (data_size >= HEADER_VANILLA_SIZE) {
		return CLOD_RHDR_VANILLA;
	}

	return CLOD_RHDR_UNKNOWN;
}


*/