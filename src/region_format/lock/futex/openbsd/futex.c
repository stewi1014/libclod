#include "../../futex.h"
#include "endian_big.h"
#include <sys/time.h>
#include <sys/futex.h>
#include <errno.h>

bool ftx_wait(uint32_t *ptr, uint32_t expected, int timeout_ms) {
	struct timespec ts;
	ts.tv_sec = timeout_ms / 1000;
	ts.tv_nsec = (timeout_ms - (ts.tv_sec * 1000)) * 1000 * 1000;
	const int res = futex(ptr, FUTEX_WAIT, expected, &ts, nullptr);
	if (res < 0) switch (errno) {
		case EAGAIN: case ETIMEDOUT: return true;
		default: return false;
	}
	return true;
}

bool ftx_wake_one(uint32_t *ptr) {
	return futex(ptr, FUTEX_WAKE, 1, nullptr, nullptr) >= 0;
}

enum ftx_result ftx_wake_all(uint32_t *ptr) {
	return futex(ptr, FUTEX_WAKE, INT_MAX, nullptr, nullptr) >= 0;
}
