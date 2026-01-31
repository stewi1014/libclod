#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>

#define check(expr) ((expr) ? (void)0 : \
	(fprintf(stderr, __FILE__":%d; Check "#expr" failed.", __LINE__), exit(1))\
)

#endif
