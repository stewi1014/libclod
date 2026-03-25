/**
 * @file lib.h
 * @internal
 *
 * @note This is used both internally and in the public header.
 * Needs to not just work with compilers the library supports,
 * but any potential toolchains that might include the public headers and use the library.
 */
#ifndef LIBCLOD_LIB_H
#define LIBCLOD_LIB_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#if defined(_WIN32)
	#ifdef CLOD_BUILD_STATIC
		#define CLOD_API
	#elifdef CLOD_BUILD_SHARED
		#define CLOD_API __declspec(dllexport)
	#else
		#define CLOD_API __declspec(dllimport)
	#endif
#elif defined(__unix__) || defined(__APPLE__)
	#if CLOD_BUILD_STATIC
		#define CLOD_API
	#else
		#define CLOD_API __attribute__((visibility("default")))
	#endif
#endif

#if defined(__GNUC__) // GCC and Clang
	#define CLOD_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
	#define CLOD_PRINTF(fmt_arg, var_args) __attribute__((format(printf, fmt_arg, var_args)))
	#define CLOD_USE_RETURN __attribute__((warn_unused_result))
	#define CLOD_CONST __attribute__((const))
	#define CLOD_PURE __attribute__((pure))
#else
	#define CLOD_NONNULL(...)
	#define CLOD_PRINTF(fmt_arg, var_args)
	#define CLOD_USE_RETURN
	#define CLOD_CONST
	#define CLOD_PURE
#endif

#endif
