#include "error.h"
#include "region_impl.h"

bool is_vanilla_compatible(const struct clod_region_opts *opts) {
	return
		opts->dims == 2 &&
		memcmp(opts->prefix, "region", strlen("region")) == 0 &&
		(
			memcmp(opts->region_ext, "mca", strlen("mca")) == 0 ||
			memcmp(opts->region_ext, "mcr", strlen("mcr")) == 0
		);
}
enum clod_region_result read_opts(struct clod_region_opts *dst, const struct clod_region_opts *src) {
	if (src->dims) {
		if (src->dims > CLOD_REGION_DIMENSIONS_MAX) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.dims %d. Must be <= %d.", src->dims,
				CLOD_REGION_DIMENSIONS_MAX);
		}
		dst->dims = src->dims;
	} else {
		dst->dims = 2;
	}

	if (src->mode) {
		if (src->mode != CLOD_REGION_MODE_RDWR && src->mode != CLOD_REGION_MODE_RDONLY) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.mode %d. Must be CLOD_REGION_MODE_RDWR or CLOD_REGION_MODE_RDONLY.",
				src->mode);
		}
		dst->mode = src->mode;
	} else {
		dst->mode = CLOD_REGION_MODE_RDWR;
	}

	if (src->unix_fd) {
		if (src->unix_fd < 0) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.unix_fd %d. Must be >= 0.",
				src->unix_fd);
		}
		dst->unix_fd = src->unix_fd;
	}

	if (src->unix_file_perms) {
		dst->unix_file_perms = src->unix_file_perms;
	}

	if (src->prefix[0]) {
		if (memchr(src->prefix, '.', CLOD_REGION_PREFIX_MAX) != nullptr) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.prefix %s. Must not contain a '.' character.",
				src->prefix);
		}
		strncpy(dst->prefix, src->prefix, CLOD_REGION_PREFIX_MAX);
		dst->prefix[CLOD_REGION_PREFIX_MAX] = '\0';
	} else {
		strncpy(dst->prefix, "region", CLOD_REGION_PREFIX_MAX + 1);
	}

	if (src->region_ext[0]) {
		strncpy(dst->region_ext, src->region_ext, CLOD_REGION_EXTENSION_MAX);
		dst->region_ext[CLOD_REGION_EXTENSION_MAX] = '\0';
	} else {
		strncpy(dst->region_ext, "mcr", CLOD_REGION_EXTENSION_MAX + 1);
	}

	if (src->chunk_ext[0]) {
		strncpy(dst->chunk_ext, src->chunk_ext, CLOD_REGION_EXTENSION_MAX);
		dst->chunk_ext[CLOD_REGION_EXTENSION_MAX] = '\0';
	} else {
		strncpy(dst->chunk_ext, "mcc", CLOD_REGION_EXTENSION_MAX + 1);
	}

	if (src->compression) {
		if (!clod_compression_support(src->compression)) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.compression %d. Either the required compression library has been intentionally disabled, or the compression mode is invalid.",
				src->compression);
		}
		dst->compression = src->compression;
	} else if (is_vanilla_compatible(dst)) {
		if (!clod_compression_support(CLOD_ZLIB)) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"libdeflate has been disabled, but it is required to compress/decompress minecraft-compatible region files.");
		}
	} else {
		if (clod_compression_support(CLOD_LZ4F)) {
			dst->compression = CLOD_LZ4F;
		} else {
			dst->compression = CLOD_UNCOMPRESSED;
		}
	}

	return true;
}