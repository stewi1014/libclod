#ifndef LIBCLOD_CLOD_DEBUG_H
#define LIBCLOD_CLOD_DEBUG_H

#include <clod/lib.h>
#include <clod/stream.h>
#include <clod/string.h>
#include <clod/sys/sys.h>

static inline int debug_print(
	const char *context,
	const char *file,
	const int line,
	const char *func,
	const char *msg,
	...
) {
	struct clod_string buff = CLOD_STRING_NEW(512);
	clod_string_cat(&buff, clod_string_from_cstr(context));
	clod_string_cat(&buff, CLOD_STRING_C(":"));
	clod_string_cat(&buff, clod_string_from_cstr(file));
	clod_string_cat(&buff, CLOD_STRING_C(":"));
	clod_string_put_uint(&buff, (uintmax_t)line, CLOD_STRING_DIGIT_ALPHABET, 10, 0, 0);
	clod_string_cat(&buff, CLOD_STRING_C(" "));
	clod_string_cat(&buff, clod_string_from_cstr(func));
	clod_string_cat(&buff, CLOD_STRING_C("():\n\t"));

	va_list args;
	va_start(args, msg);
	clod_string_vformat(&buff, clod_string_from_cstr(msg), args);
	va_end(args);

	if (buff.len == buff.cap) buff.len--;
	buff.ptr[buff.len++] = '\n';

	clod_stream_stderr->write(clod_stream_stderr, &buff);
	return 1;
}

#if defined(__GNUC__)
#define _debug_assume(expr) ((expr) ? (void)0 : __builtin_unreachable())
#elif defined(_MSC_VER)
#define _debug_assume(expr) __assume(expr)
#endif

#define CLOD_TEST 1

#define debug(context, msg, ...) (!(context) ? (void)0 : (debug_print(#context, __FILE__, __LINE__, __func__, msg __VA_OPT__(,) __VA_ARGS__), (void)0))
#define fatal(context, msg, ...) (!(context) ? (void)0 : (debug_print(#context, __FILE__, __LINE__, __func__, msg __VA_OPT__(,) __VA_ARGS__), clod_exit(1), _debug_assume(0)))
#define assert_fatal(context, expr, msg, ...) (!(context) ? _debug_assume(expr) : (expr) ? (void)0 : (debug_print(#context, __FILE__, __LINE__, __func__, "Assertion \""#expr"\" failed: "msg __VA_OPT__(,) __VA_ARGS__), clod_exit(1), _debug_assume(0)))

#endif
