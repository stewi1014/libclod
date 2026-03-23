#ifndef LIBCLOD_SYSCALL_H
#define LIBCLOD_SYSCALL_H

#include <stdio.h>
#include <clod/lib.h>
#include <linux/time.h>

int syscall_futex_wait(const int *addr, int expected, struct timespec *timeout);
int syscall_futex_wake(const int *addr, int num);

ssize_t syscall_read(int fd, void *buff, size_t size);
ssize_t syscall_write(int fd, const void *buff, size_t size);
int syscall_close(int fd);

void *syscall_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int syscall_munmap(void *addr, size_t length);

#endif
