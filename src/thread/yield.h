#ifndef LIBCLOD_YIELD_H
#define LIBCLOD_YIELD_H

#include "clod_config.h"

#if CLOD_HAVE_SCHED_YIELD
	#include <sched.h>
	#define clod_yield sched_yield
#else
	#error "Yield not implemented on this platform"
#endif

#if CLOD_HAVE_X86
	#include <immintrin.h>
	#define clod_pause _mm_pause
#elif CLOD_HAVE_ARM
	#include <arm_acle.h>
	#define clod_pause __yield()
#elif
	#error "Pause instruction not implemented on this architecture"
#endif

#endif
