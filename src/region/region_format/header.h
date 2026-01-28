#ifndef CLOD_HEADER_H
#define CLOD_HEADER_H

#include <clod/big_endian.h>
#include <clod/region.h>

// The maximum size of the mutex implementation across all target platforms.
#define MUTEX_MAX_SIZE 40

#define HEADER_MAGIC \
	"\n\n"\
	"libclod extended region file format version 1.\n"\
	"See github.com/stewi1014/clod for format details.\n"\
	"\n"

static_assert(sizeof(HEADER_MAGIC) <= 128);

// dimension count -> region file extent mapping.
// libclod region files each store a hypercube of space,
// the width of the hypercube being given by this constant.
// Values are chosen to ensure the number of chunks in each region file is close to 1024.
static constexpr size_t extent[CLOD_REGION_DIMENSIONS_MAX + 1] = {
	[1] = 1024,
	[2] = 32,
	[3] = 10,
	[4] = 5,
	[5] = 4,
	[6] = 3,
	[7] = 2,
	[8] = 2,
	[9] = 2,
	[10] = 2
};

// dimension count -> number of chunks in each region file.
// It is extent^dimension_count.
// put another way - it is the volume of the hypercube each region file stores.
static constexpr size_t cardinality[CLOD_REGION_DIMENSIONS_MAX + 1] = {
	[1] = 1024,
	[2] = 1024,
	[3] = 1000,
	[4] = 625,
	[5] = 1024,
	[6] = 729,
	[7] = 128,
	[8] = 256,
	[9] = 512,
	[10] = 1024
};

// The vanilla header.
// Used when reading (read-only mode) and migrating vanilla region files.
struct vanilla_header {
	struct {
		// Offset in sectors (4096 bytes) of the chunk data.
		beu24 offset_sectors;
		// Size in sectors (4096 bytes) of the chunk data.
		beu8 size_sectors;
	} chunk_locations[1024];
	struct {
		// Last modification time in seconds since unix epoch.
		beu32 timestamp;
	} chunk_timestamps[1024];
};

// The libclod header.
// Used for all region files by default.
struct libclod_header {
	union { char _reserved[256]; struct {
		// Unique string identifying the format.
		char magic[128];
		// Checksum of nbt data.
		beu32 nbt_checksum;
		// Size of nbt data.
		beu32 nbt_size;
	};};
	// nbt structure containing region file metadata.
	char nbt_data[];
};

// The vanilla header followed by libclod header.
// Used if it's likely that backwards compatability with vanilla will be desired.
struct compound_header {
	struct vanilla_header vanilla;
	struct libclod_header libclod;
};

#endif
