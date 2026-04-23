#include "config.h"
#include "syscall.h"
#include <clod/sys/sys.h>
#include <linux/time.h>
#include <linux/unistd.h>
#include <linux/signal.h>

#ifdef CLOD_HAVE_X86_64

long syscall0(long number) {
	asm volatile(
		"syscall\n\t"
		: "+a" (number)
		:
		: "memory", "cc", "r11", "rcx"
	);
	return number;
}

long syscall1(long _1, long number) {
	register long r1 asm ("rdi") = _1;

	asm volatile(
		"syscall\n\t"
		: "+a" (number)
		: "D" (r1)
		: "memory", "cc", "r11", "rcx"
	);
	return number;
}

long syscall2(long _1, long _2, long number) {
	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;

	asm volatile(
		"syscall\n\t"
		: "+a" (number)
		: "D" (r1), "S" (r2)
		: "memory", "cc", "r11", "rcx"
	);
	return number;
}

long syscall3(long _1, long _2, long _3, long number) {
	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;
	register long r3 asm ("rdx") = _3;

	asm volatile(
		"syscall\n\t"
		: "+a" (number)
		: "D" (r1), "S" (r2), "d" (r3)
		: "memory", "cc", "r11", "rcx"
	);
	return number;
}

long syscall4(long _1, long _2, long _3, long _4, long number) {
	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;
	register long r3 asm ("rdx") = _3;
	register long r4 asm ("r10") = _4;

	asm volatile(
		"syscall\n\t"
		: "+a" (number)
		: "D" (r1), "S" (r2), "d" (r3), "r" (r4)
		: "memory", "cc", "r11", "rcx"
	);
	return number;
}

long syscall5(long _1, long _2, long _3, long _4, long _5, long number) {
	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;
	register long r3 asm ("rdx") = _3;
	register long r4 asm ("r10") = _4;
	register long r5 asm ("r8") = _5;

	asm volatile(
		"syscall\n\t"
		: "+a" (number)
		: "D" (r1), "S" (r2), "d" (r3), "r" (r4), "r" (r5)
		: "memory", "cc", "r11", "rcx"
	);
	return number;
}

long syscall6(long _1, long _2, long _3, long _4, long _5, long _6, long number) {
	register long r1 asm ("rdi") = _1;
	register long r2 asm ("rsi") = _2;
	register long r3 asm ("rdx") = _3;
	register long r4 asm ("r10") = _4;
	register long r5 asm ("r8") = _5;
	register long r6 asm ("r9") = _6;

	asm volatile(
		"syscall\n\t"
		: "+a" (number)
		: "D" (r1), "S" (r2), "d" (r3), "r" (r4), "r" (r5), "r" (r6)
		: "memory", "cc", "r11", "rcx"
	);
	return number;
}

#else
#error "Linux syscalls not implemented on this architecture"
#endif

void clod_exit(const int code) {
	//#if CLOD_DEBUG
		clod_debugbreak();
	//#endif
	syscall(__NR_exit, code);
	__builtin_unreachable();
}

void clod_debugbreak() {
	const long pid = syscall(__NR_getpid);
	syscall(__NR_kill, pid, SIGTRAP);
}
