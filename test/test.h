#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>

#define test_check(expr, message, ...) if ((expr) ? false :\
	(fprintf(stderr, __FILE__":%d Test failure; "message" ("#expr")\n", __LINE__ __VA_OPT__(,) __VA_ARGS__), true)\
)

#define check(message, expr) ((expr) ? (void)0 : \
	(fprintf(stderr, __FILE__":%d Test failure; "message" ("#expr")\n", __LINE__), __builtin_trap())\
)

#endif
