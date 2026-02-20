#ifndef LIBCLOD_HEADER_H
#define LIBCLOD_HEADER_H

#include "format.h"
#include <stdint.h>

void magic_set(struct clod_rfmt *rfmt, const char *magic);
bool magic_equals(struct clod_rfmt *rfmt, const char *magic);

#endif