
#include <clod/region_format.h>
#include "futex.h"
#include "atomic.h"
#include <time.h>

static uint32_t timespec_diff_ms(const struct timespec from, const struct timespec to) {
	return (uint32_t)(
		(to.tv_sec - from.tv_sec) * 1000 +
		(to.tv_nsec / 1000000 - from.tv_nsec / 1000000)
	);
}

#define BLOCKED UINT32_C(1)
#define LOCKED  UINT32_C(2)
#define ACQUIRE UINT32_C(4)
#define NUMBER  UINT32_C(8)
#define NUMBER_MASK UINT32_C(~7)

