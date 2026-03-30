#ifndef LIBCLOD_SYSCALL_H
#define LIBCLOD_SYSCALL_H

#include <clod/lib.h>
#include <linux/time.h>
#include <linux/unistd.h>
#include <linux/errno.h>

long syscall0(int number);
long syscall1(long _1, int number);
long syscall2(long _1, long _2, int number);
long syscall3(long _1, long _2, long _3, int number);
long syscall4(long _1, long _2, long _3, long _4, int number);
long syscall5(long _1, long _2, long _3, long _4, long _5, int number);
long syscall6(long _1, long _2, long _3, long _4, long _5, long _6, int number);

#define _syscall_switch(_1, _2, _3, _4, _5, _6, N, ...) N
#define syscall(number, ...) _syscall_switch(__VA_ARGS__, syscall6, syscall5, syscall4, syscall3, syscall2, syscall1, syscall0) (__VA_ARGS__ __VA_OPT__(,) number)

[[deprecated]]
int syscall_futex_wait(const int *addr, int expected, struct timespec *timeout);
[[deprecated]]
int syscall_futex_wake(const int *addr, int num);

[[deprecated]]
long syscall_read(int fd, void *buff, size_t size);
[[deprecated]]
long syscall_write(int fd, const void *buff, size_t size);
[[deprecated]]
int syscall_close(int fd);

[[deprecated]]
void *syscall_mmap(void *addr, size_t length, int prot, int flags, int fd, long offset);
[[deprecated]]
int syscall_munmap(void *addr, size_t length);

#endif
