#include "config.h"
#include "syscall.h"
#include <clod/sys/sys.h>
#include <linux/time.h>
#include <linux/unistd.h>

#ifdef CLOD_HAVE_X86_64

long syscall0(int number) {
	long res;
	asm volatile(
		"syscall\n\t"
		: "=a" (res)
		: "0" (number)
		: "memory", "cc", "r11", "rcx"
	);
	return res;
}

long syscall1(long _1, int number) {
	long res;

	register long r1 asm ("rdi") = _1;

	asm volatile(
		"syscall\n\t"
		: "=a" (res)
		: "0" (number), "r" (r1)
		: "memory", "cc", "r11", "rcx"
	);
	return res;
}

long syscall2(long _1, long _2, int number) {
	long res;

	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;

	asm volatile(
		"syscall\n\t"
		: "=a" (res)
		: "0" (number), "r" (r1), "r" (r2)
		: "memory", "cc", "r11", "rcx"
	);
	return res;
}

long syscall3(long _1, long _2, long _3, int number) {
	long res;

	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;
	register long r3 asm ("rdx") = _3;

	asm volatile(
		"syscall\n\t"
		: "=a" (res)
		: "0" (number), "r" (r1), "r" (r2), "r" (r3)
		: "memory", "cc", "r11", "rcx"
	);
	return res;
}

long syscall4(long _1, long _2, long _3, long _4, int number) {
	long res;

	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;
	register long r3 asm ("rdx") = _3;
	register long r4 asm ("r10") = _4;

	asm volatile(
		"syscall\n\t"
		: "=a" (res)
		: "0" (number), "r" (r1), "r" (r2), "r" (r3), "r" (r4)
		: "memory", "cc", "r11", "rcx"
	);
	return res;
}

long syscall5(long _1, long _2, long _3, long _4, long _5, int number) {
	long res;

	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;
	register long r3 asm ("rdx") = _3;
	register long r4 asm ("r10") = _4;
	register long r5 asm ("r8") = _5;

	asm volatile(
		"syscall\n\t"
		: "=a" (res)
		: "0" (number), "r" (r1), "r" (r2), "r" (r3), "r" (r4), "r" (r5)
		: "memory", "cc", "r11", "rcx"
	);
	return res;
}

long syscall6(long _1, long _2, long _3, long _4, long _5, long _6, int number) {
	long res;

	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;
	register long r3 asm ("rdx") = _3;
	register long r4 asm ("r10") = _4;
	register long r5 asm ("r8") = _5;
	register long r6 asm ("r9") = _6;

	asm volatile(
		"syscall\n\t"
		: "=a" (res)
		: "0" (number), "r" (r1), "r" (r2), "r" (r3), "r" (r4), "r" (r5), "r" (r6)
		: "memory", "cc", "r11", "rcx"
	);
	return res;
}

#define _syscall_switch(_1, _2, _3, _4, _5, _6, N, ...) N
#define syscall(number, ...) _syscall_switch(__VA_ARGS__, syscall6, syscall5, syscall4, syscall3, syscall2, syscall1, syscall0) (__VA_ARGS__ __VA_OPT__(,) number)

#else
#error "Linux syscalls not implemented on this architecture"
#endif

int syscall_futex_wait(const int *addr, const int expected, struct timespec *timeout) {
	return (int)syscall(__NR_futex, (long)addr, __NR_futex_wait, expected, (long)timeout);
}

int syscall_futex_wake(const int *addr, const int num) {
	return (int)syscall(__NR_futex, (long)addr, __NR_futex_wake, num);
}

long syscall_read(const int fd, void *buff, const size_t size) {
	return syscall(__NR_read, fd, (long)buff, (long)size);
}

long syscall_write(const int fd, const void *buff, const size_t size) {
	return syscall(__NR_write, fd, (long)buff, (long)size);
}

int syscall_close(const int fd) {
	return (int)syscall(__NR_close, fd);
}

void *syscall_mmap(void *addr, const size_t length, const int prot, const int flags, const int fd, const long offset) {
	return (void*)syscall(__NR_mmap, (long)addr, (long)length, prot, flags, fd, offset);
}

int syscall_munmap(void *addr, const size_t length) {
	return (int)syscall(__NR_munmap, (long)addr, (long)length);
}

void clod_exit(const int code) {
	syscall(__NR_exit, code);
	__builtin_unreachable();
}
