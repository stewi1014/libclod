#ifndef LIBCLOD_FORMAT_H
#define LIBCLOD_FORMAT_H

#include <clod/region_format.h>
#include <clod/table.h>
#include "bitarray.h"
#include <stdint.h>

#define LIBCLOD_MAGIC \
	"\n\n"\
	"libclod extended region file format version 1.\n"\
	"See github.com/stewi1014/clod for format details.\n"\
	"\n\n"

constexpr size_t MAGIC_LEN = sizeof(LIBCLOD_MAGIC);
constexpr size_t HEADER_SIZE = 20480u;

struct clod_rfmt {
	struct clod_rfmt_opts opts;

	uint8_t *data;
	size_t data_size;

#ifndef NDEBUG
	bitarray(1024) held_locks;
	bitarray(1024) observing_locks;
#endif
};

typedef struct {
	uint32_t offset : 24;
	uint8_t size : 8;
} chunk_loc;

enum clod_rfmt_result chunk_lock_acquire(struct clod_rfmt *rfmt, unsigned chunk_index);
enum clod_rfmt_result chunk_lock_refresh(struct clod_rfmt *rfmt, unsigned chunk_index);
enum clod_rfmt_result chunk_lock_release(struct clod_rfmt *rfmt, unsigned chunk_index);

enum clod_rfmt_result global_lock_acquire(struct clod_rfmt *rfmt);
enum clod_rfmt_result global_lock_refresh(struct clod_rfmt *rfmt);
enum clod_rfmt_result global_lock_release(struct clod_rfmt *rfmt);

enum clod_rfmt_result resize_lock_acquire(struct clod_rfmt *rfmt, size_t *old_size, size_t grow_size);
enum clod_rfmt_result resize_lock_release(struct clod_rfmt *rfmt);

#endif
