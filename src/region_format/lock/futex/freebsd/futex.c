#include "../../futex.h"
#include "endian_big.h"

// TODO
// https://man.freebsd.org/cgi/man.cgi?query=_umtx_op
#error "Not implemented yet"

enum ftx_result ftx_wait(uint32_t *ptr, uint32_t expected, int timeout_ms) {

}

enum ftx_result ftx_wake_one(uint32_t *ptr) {

}

enum ftx_result ftx_wake_all(uint32_t *ptr) {

}
