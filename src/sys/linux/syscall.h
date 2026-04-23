#ifndef LIBCLOD_SYSCALL_H
#define LIBCLOD_SYSCALL_H

#include <clod/lib.h>
#include <linux/time.h>
#include <linux/unistd.h>
#include <linux/errno.h>

long syscall0(long number);
long syscall1(long _1, long number);
long syscall2(long _1, long _2, long number);
long syscall3(long _1, long _2, long _3, long number);
long syscall4(long _1, long _2, long _3, long _4, long number);
long syscall5(long _1, long _2, long _3, long _4, long _5, long number);
long syscall6(long _1, long _2, long _3, long _4, long _5, long _6, long number);

#define _syscall_switch(_1, _2, _3, _4, _5, _6, N, ...) N
#define syscall(number, ...) _syscall_switch(__VA_ARGS__ __VA_OPT__(,) syscall6, syscall5, syscall4, syscall3, syscall2, syscall1, syscall0) (__VA_ARGS__ __VA_OPT__(,) number)

#endif
