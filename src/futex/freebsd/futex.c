#include "clod/futex.h"

// TODO
// https://man.freebsd.org/cgi/man.cgi?query=_umtx_op
#error "FreeBSD futex not plugged in"

bool ftx_wait(const uint32_t *ptr, uint32_t expected, int timeout_ms) {

}

void ftx_wake_one(const uint32_t *ptr) {

}

void ftx_wake_all(const uint32_t *ptr) {

}
