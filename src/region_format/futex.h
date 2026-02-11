#ifndef LIBCLOD_PLATFORM_H
#define LIBCLOD_PLATFORM_H

#include <stdint.h>
#include "endian_big.h"

enum ftx_result {
	FTX_OK,
	FTX_TIMEOUT,
	FTX_OTHER
};

/// Futex wait.
enum ftx_result ftx_wait(uint32_t *ptr, uint32_t expected, uint32_t wait_ms);
/// Wake one thread waiting on the futex.
enum ftx_result ftx_wake_one(uint32_t *ptr);
/// Wake all threads waiting on the futex.
enum ftx_result ftx_wake_all(uint32_t *ptr);


/// Futex wait on big-endian value.
static inline enum ftx_result ftx_wait_be(uint32_t *ptr_be, uint32_t expected, uint32_t wait_ms) {
	uint32_t expected_be;
	beu32_enc((uint8_t*)&expected_be, expected);
	return ftx_wait(ptr_be, expected_be, wait_ms);
}

#endif
