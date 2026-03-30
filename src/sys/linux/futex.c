#include <clod/sys/futex.h>
#include <linux/time.h>
#include <linux/errno.h>
#include "syscall.h"

enum clod_futex_error clod_futex_wait(const int *ptr, const int expected, int64_t timeout_us) {
	struct timespec ts;
	ts.tv_sec = timeout_us / 1000000;
	ts.tv_nsec = (timeout_us - ts.tv_sec * 1000000) * 1000;
	const long res = syscall(__NR_futex, (long)ptr, __NR_futex_wait, expected, (long)&ts);
	if (res < 0) {
		if (-res == ETIMEDOUT) return CLOD_FUTEX_OK;
		if (-res == EAGAIN) return CLOD_FUTEX_OK;
		if (-res == EINTR) return CLOD_FUTEX_INTERRUPT;
		return CLOD_FUTEX_INVALID;
	}
	return CLOD_FUTEX_OK;
}

enum clod_futex_error clod_futex_wake_one(const int *ptr) {
	const long res = syscall(__NR_futex, (long)ptr, __NR_futex_wake, 1);
	if (res < 0) {
		return CLOD_FUTEX_INVALID;
	}
	return CLOD_FUTEX_OK;
}

enum clod_futex_error clod_futex_wake_all(const int *ptr) {
	const long res = syscall(__NR_futex, (long)ptr, __NR_futex_wake, (long)INT_MAX);
	if (res < 0) {
		return CLOD_FUTEX_INVALID;
	}
	return CLOD_FUTEX_OK;
}
