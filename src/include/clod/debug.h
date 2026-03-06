#ifndef LIBCLOD_CLOD_DEBUG_H
#define LIBCLOD_CLOD_DEBUG_H

#include <clod/lib.h>
#include <stdio.h>

#define debug(context, msg, ...) ((context) ? fprintf(stderr, #context":"__FILE__":%d: "msg, __LINE__ __VA_OPT__(,) __VA_ARGS__) : 0)
#define fatal(context, msg, ...) (fprintf(stderr, #context":"__FILE__":%d: "msg, __LINE__ __VA_OPT__(,) __VA_ARGS__), exit(1))

#endif
