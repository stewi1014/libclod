#ifndef LIBCLOD_SYS_H
#define LIBCLOD_SYS_H

#include <clod/lib.h>

CLOD_API CLOD_NORETURN
void clod_exit(int code);

CLOD_API
void clod_debugbreak();

#endif
