#ifndef LIBCLOD_FUTEX_H
#define LIBCLOD_FUTEX_H

#include <stdint.h>

/// Futex wait.
/// @return True if timeout was reached, false on normal wakeup.
bool ftx_wait(const uint32_t *ptr, uint32_t expected, int timeout_ms);
/// Wake one thread waiting on the futex.
void ftx_wake_one(const uint32_t *ptr);
/// Wake all threads waiting on the futex.
void ftx_wake_all(const uint32_t *ptr);

#endif
