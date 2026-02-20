#ifndef LIBCLOD_PLATFORM_H
#define LIBCLOD_PLATFORM_H

#include <stdint.h>

/// Futex wait.
bool ftx_wait(const uint32_t *ptr, uint32_t expected, int timeout_ms);
/// Wake one thread waiting on the futex.
bool ftx_wake_one(const uint32_t *ptr);
/// Wake all threads waiting on the futex.
bool ftx_wake_all(const uint32_t *ptr);

#endif
