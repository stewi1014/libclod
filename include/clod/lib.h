/**
 * @file lib.h
 * @internal
 *
 * @note This is used both internally and in the public header.
 * Needs to not just work with compilers the library supports,
 * but any potential toolchains that might include the public headers and use the library.
 */
#ifndef CLOD_LIB_H
#define CLOD_LIB_H

#if defined(__GNUC__) // GCC and Clang
	#if defined(_WIN32) // Windows - Clang
		#define CLOD_API __declspec(dllexport)
	#elif defined(__unix__) || defined(__APPLE__) // Unix - GCC/Clang
		#define CLOD_API __attribute__((visibility("default")))
	#else
		#error "Unknown platform"
	#endif

	#define CLOD_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
	#define CLOD_USE_RETURN __attribute__((warn_unused_result))
	#define CLOD_CONST __attribute__((const))
	#define CLOD_PURE __attribute__((pure))
	#define CLOD_INLINE __attribute__ ((always_inline))
#else
	#error "Unknown toolchain"
#endif

#endif
