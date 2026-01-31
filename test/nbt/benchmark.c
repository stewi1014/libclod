#include "test.h"
#include <clod/nbt.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

const char level_data[] = {
#embed "player.nbt"
};

#define ITER_COUNT 20000
#define NS_IN_SEC 1000000000
#define BYTE_IN_GB 1000000000

int main() {
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	volatile size_t total = 0;

	for (int i = 0; i < ITER_COUNT; i++) {
		total += clod_nbt_tag_size((void*)level_data, level_data + sizeof(level_data));
	}

	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);

	const uint64_t ns_diff = end.tv_nsec - start.tv_nsec +
		(end.tv_sec - start.tv_sec) * NS_IN_SEC;

	printf("%"PRIu64" ns/parse\n", ns_diff / ITER_COUNT);
	printf("%0.3f ns/byte\n", (double)ns_diff / (double)total);
	printf("%0.3f GB/s\n", (double)total * NS_IN_SEC / BYTE_IN_GB / (double)ns_diff);
}
