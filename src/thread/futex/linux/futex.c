#include "clod_thread_config.h"
#include "clod/debug.h"
#include <clod/thread.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#if CLOD_HAVE_X86_64 && CLOD_NATIVE

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlanguage-extension-token"

static long wait(const int *ptr, const int expected, const struct timespec *timeout) {
	register long rax asm("rax") = SYS_futex;
	register long rdi asm("rdi") = (long)ptr;
	register long rsi asm("rsi") = FUTEX_WAIT;
	register long rdx asm("rdx") = expected;
	register long r10 asm("r10") = (long)timeout;
	asm volatile(
		"syscall"
		: "=r"(rax)
		: "r"(rax), "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10)
		: "rcx", "r11", "memory"
	);
	if (rax < 0) return -rax;
	return 0;
}

static long wake(const int *ptr, const int count) {
	register long rax asm("rax") = SYS_futex;
	register long rdi asm("rdi") = (long)ptr;
	register long rsi asm("rsi") = FUTEX_WAKE;
	register long rdx asm("rdx") = count;
	asm volatile(
		"syscall"
		: "=r"(rax)
		: "r"(rax), "r"(rdi), "r"(rsi), "r"(rdx)
		: "rcx", "r11", "memory"
	);
	if (rax < 0) return -rax;
	return 0;
}

#pragma GCC diagnostic pop

#else

static long wait(const int *ptr, const int expected, const struct timespec *timeout) {
	long ret = syscall(SYS_futex, ptr, FUTEX_WAIT, expected, timeout);
	if (ret < 0) return errno;
}

static long wake(const int *ptr, const int count) {
	long ret = syscall(SYS_futex, ptr, FUTEX_WAKE, count);
	if (ret < 0) return errno;
}

#endif

bool clod_futex_wait(const int *ptr, const int expected, int64_t timeout_us) {
	struct timespec ts;
	ts.tv_sec = timeout_us / 1000000;
	ts.tv_nsec = (timeout_us - ts.tv_sec * 1000000) * 1000;
	auto const res = wait(ptr, expected, &ts);
	if (res != 0) {
		if ((res == EAGAIN) || (res == EWOULDBLOCK)) return false; // value changed
		if (res == ETIMEDOUT) return true; // timeout
		debug(CLOD_DEBUG_THREAD, "attempting to wait on futex: %ld %s\n", res, strerror((int)res));
	}
	return false;
}

void clod_futex_wake_one(const int *ptr) {
	auto const res = wake(ptr, 1);
	if (res != 0) {
		debug(CLOD_DEBUG_THREAD, "attempting to wake one futex waiter: %ld %s\n", res, strerror((int)res));
	}
}

void clod_futex_wake_all(const int *ptr) {
	auto const res = wake(ptr, INT_MAX);
	if (res != 0) {
		debug(CLOD_DEBUG_THREAD, "attempting to wake all futex waiters: %ld %s\n", res, strerror((int)res));
	}
}
