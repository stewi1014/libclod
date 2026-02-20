#include "../../futex.h"
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

bool ftx_wait(const uint32_t *ptr, uint32_t expected, int timeout_ms) {
	struct timespec ts;
	ts.tv_sec = timeout_ms / 1000;
	ts.tv_nsec = (timeout_ms - (ts.tv_sec * 1000)) * 1000 * 1000;
	const long res = syscall(SYS_futex, ptr, FUTEX_WAIT, expected, &ts);
	if (res < 0) switch (errno) {
		case EAGAIN: case ETIMEDOUT: return true;
		default: return false;
	}
	return true;
}

bool ftx_wake_one(const uint32_t *ptr) {
	return syscall(SYS_futex, ptr, FUTEX_WAKE, 1) >= 0;
}

bool ftx_wake_all(const uint32_t *ptr) {
	return syscall(SYS_futex, ptr, FUTEX_WAKE, INT_MAX) >= 0;
}
