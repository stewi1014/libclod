#include "../../futex.h"
#include <os/sync.h>
#include <errno.h>


enum ftx_result ftx_wait(uint32_t *ptr, uint32_t expected, uint32_t wait_ms) {
	const int res = os_sync_wait_on_address_with_timeout(
		ptr, expected, sizeof(*ptr), OS_SYNC_WAIT_ON_ADDRESS_SHARED, OS_CLOCK_MACH_ABSOLUTE_TIME, (uint64_t)wait_ms * 1000 * 1000
	);

	if (res < 0) switch (errno) {
		case ETIMEDOUT: return FTX_TIMEOUT;
		default: return FTX_OTHER;
	}
	return FTX_OK;
}

enum ftx_result ftx_wake_one(uint32_t *ptr) {
	const int res = os_sync_wake_by_address_any(
		ptr, sizeof(*ptr), OS_SYNC_WAKE_BY_ADDRESS_SHARED
	);
	if (res < 0) switch (errno) {
		case ENOENT: return FTX_OK;
		default: return FTX_OTHER;
	}
	return FTX_OK;
}

enum ftx_result ftx_wake_all(uint32_t *ptr) {
	const int res = os_sync_wake_by_address_all(
		ptr, sizeof(*ptr), OS_SYNC_WAKE_BY_ADDRESS_SHARED
	);
	if (res < 0) switch (errno) {
		case ENOENT: return FTX_OK;
		default: return FTX_OTHER;
	}
	return FTX_OK;
}
