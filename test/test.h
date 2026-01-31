#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>

#define check(message, expr) ((expr) ? (void)0 : \
	(fprintf(stderr, __FILE__":%d; Test failure; "message" ("#expr")\n", __LINE__), exit(1))\
)

#endif
