#include "clod/futex.h"
#include <linux/futex.h>
#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

bool ftx_wait(const uint32_t *ptr, const uint32_t expected, const int timeout_ms) {
	struct timespec ts;
	ts.tv_sec = timeout_ms / 1000;
	ts.tv_nsec = (timeout_ms - (ts.tv_sec * 1000)) * 1000 * 1000;
	auto const res = syscall(SYS_futex, ptr, FUTEX_WAIT, expected, &ts);
	if (res < 0) {
		if ((errno == EAGAIN) | (errno == EWOULDBLOCK)) return false; // value changed
		if (errno == ETIMEDOUT) return true; // timeout
		perror("Attempting to wait on futex");
		exit(EXIT_FAILURE);
	}
	return false;
}

void ftx_wake_one(const uint32_t *ptr) {
	auto const res = syscall(SYS_futex, ptr, FUTEX_WAKE, 1);
	if (res < 0) {
		perror("Attempting to wake one futex waiter");
		exit(EXIT_FAILURE);
	}
}

void ftx_wake_all(const uint32_t *ptr) {
	auto const res = syscall(SYS_futex, ptr, FUTEX_WAKE, INT_MAX);
	if (res < 0) {
		perror("Attempting to wake all futex waiters");
		exit(EXIT_FAILURE);
	}
}
