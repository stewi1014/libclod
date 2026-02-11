#include "../../futex.h"
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

enum ftx_result ftx_wait(uint32_t *ptr, uint32_t expected, uint32_t wait_ms) {
	struct timespec ts;
	ts.tv_sec = wait_ms / 1000;
	ts.tv_nsec = (wait_ms - (ts.tv_sec * 1000)) * 1000 * 1000;
	const long res = syscall(SYS_futex, ptr, FUTEX_WAIT, expected, &ts);
	if (res < 0) switch (errno) {
		case EAGAIN: return FTX_OK;
		case ETIMEDOUT: return FTX_TIMEOUT;
		default: return FTX_OTHER;
	}
	return FTX_OK;
}

enum ftx_result ftx_wake_one(uint32_t *ptr) {
	const long res = syscall(SYS_futex, ptr, FUTEX_WAKE, 1);
	if (res < 0) return FTX_OTHER;
	return FTX_OK;
}

enum ftx_result ftx_wake_all(uint32_t *ptr) {
	const long res = syscall(SYS_futex, ptr, FUTEX_WAKE, INT_MAX);
	if (res < 0) return FTX_OTHER;
	return FTX_OK;
}
