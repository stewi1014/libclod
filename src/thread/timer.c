#include "clod_config.h"
#include <clod/thread.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "yield.h"

#if CLOD_HAVE_THREADS_H
#include <threads.h>
#endif


static int64_t from_timespec(const struct timespec time) {
	return time.tv_sec * 1000 * 1000 + time.tv_nsec / 1000;
}

static struct timespec to_timespec(const int64_t us) {
	return (struct timespec){.tv_sec = us / 1000000, .tv_nsec = (us % 1000000) * 1000};
}

static int64_t now() {
#if CLOD_HAVE_CLOCK_GETTIME && CLOD_HAVE_CLOCK_MONOTONIC_RAW
	{
		struct timespec time;
		if (clock_gettime(CLOCK_MONOTONIC_RAW, &time) == 0)
			return from_timespec(time);
	}
#elif CLOD_HAVE_CLOCK_GETTIME && CLOD_HAVE_CLOCK_MONOTONIC
	{
		struct timespec time;
		if (clock_gettime(CLOCK_MONOTONIC, &time) == 0)
			return from_timespec(time);
	}
#elif CLOD_HAVE_CLOCK_MONOTONIC_RAW
	{
		struct timespec time;
		if (timespec_get(&time, CLOCK_MONOTONIC_RAW) == CLOCK_MONOTONIC_RAW)
			return from_timespec(time);
	}
#elif CLOD_HAVE_CLOCK_MONOTONIC
	{
		struct timespec time;
		if (timespec_get(&time, CLOCK_MONOTONIC) == CLOCK_MONOTONIC)
			return from_timespec(time);
	}
#else
#error "No system clock method found"
#endif
	fprintf(stderr, "CLOD_THREAD: Failed to get system clock");
	return 0;
}

int64_t clod_timer(int64_t *time, const int64_t duration_us) {
	int64_t enter = now();
	if (duration_us <= 0) {
		return enter;
	}
	int64_t end = enter + duration_us;
	if (time) {
		end = *time + duration_us;
		*time = end;
		if (enter > end) return enter;
	}

#if CLOD_HAVE_CLOCK_NANOSLEEP && CLOD_HAVE_CLOCK_MONOTONIC_RAW
	static volatile bool monotonic_raw_works = true;
	if (monotonic_raw_works) {
		const struct timespec wait = to_timespec(end - enter);
		const int res = clock_nanosleep(CLOCK_MONOTONIC_RAW, 0, &wait, nullptr);
		if (res == EINVAL && time) *time = enter;
		if (res == ENOTSUP) monotonic_raw_works = false;
		else goto done;
	}
#endif
#if CLOD_HAVE_CLOCK_NANOSLEEP && CLOD_HAVE_CLOCK_MONOTONIC
	static volatile bool monotonic_works = true;
	if (monotonic_works) {
		const struct timespec wait = to_timespec(end - enter);
		const int res = clock_nanosleep(CLOCK_MONOTONIC, 0, &wait, nullptr);
		if (res == EINVAL && time) *time = enter;
		if (res == ENOTSUP) monotonic_works = false;
		else goto done;
	}
#endif
#if CLOD_HAVE_NANOSLEEP
	{
		const struct timespec wait = to_timespec(end - enter);
		nanosleep(&wait, nullptr);
		goto done;
	}
#elif CLOD_HAVE_THREADS_H
	{
		const struct timespec wait = to_timespec(end - enter);
		thrd_sleep(&wait, nullptr);
		goto done;
	}
#endif

	int64_t wait = enter;
	while (wait < end) {
		clod_yield();
		wait = now();
	}

done:
	return enter;
}
