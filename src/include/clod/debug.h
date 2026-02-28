#ifndef LIBCLOD_CLOD_DEBUG_H
#define LIBCLOD_CLOD_DEBUG_H

#include <clod/lib.h>
#include <stdio.h>

#define debug(context, msg, ...) if (context) fprintf(stderr, #context":"__FILE__":%d: "msg, __LINE__ __VA_OPT__(,) __VA_ARGS__)

#endif
