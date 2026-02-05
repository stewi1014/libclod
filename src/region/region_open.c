#include <clod/region.h>
#include "region_impl.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>


struct clod_region *clod_region_open(const char *path, const struct clod_region_opts *opts) {
	struct clod_region *r = malloc(sizeof(struct clod_region));
	if (!r) return nullptr;

	memset(r, 0, sizeof(struct clod_region));
	if (!read_opts(&r->opts, opts)) {
		free(r);
		return nullptr;
	}

	mutex_init(&r->mtx);
	auto const res = dir_open(&r->d, path, &r->opts);
	if (res != CLOD_REGION_OK) {
		mutex_destroy(&r->mtx);
		free(r);
		return nullptr;
	}

	r->cache_len = 0;
	r->cache = nullptr;

	return r;
}
enum clod_region_result clod_region_close(struct clod_region *r) {
	if (r->inside != 0) {
		region_error(CLOD_REGION_INVALID_USAGE, "Attempted to close region while still in use.");
		exit(EXIT_FAILURE);
	}

	mutex_destroy(&r->mtx);
	auto const dir_res = dir_close(r->d);
	auto const fc_res = file_cache_destroy(r);
	free(r);
	return dir_res != CLOD_REGION_OK ? dir_res : fc_res;
}
