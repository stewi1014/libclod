#include "clod_thread_config.h"
#include <clod/thread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "yield.h"

#if CLOD_HAVE_STDTHREADS
#include <threads.h>
#endif

#define from_timespec(ts) ((ts).tv_sec * 1000 * 1000 + (ts).tv_nsec / 1000)
#define to_timespec(us) ((struct timespec){.tv_sec = (us) / 1000000, .tv_nsec = (us) % 1000000 * 1000})
struct timespec add_timespec(struct timespec ts, const int64_t us) {
	ts.tv_sec += us / 1000000;
	ts.tv_nsec += us % 1000000 * 1000;

	while (ts.tv_nsec >= 1000000000) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}

	while (ts.tv_nsec < 0) {
		ts.tv_sec--;
		ts.tv_nsec += 1000000000;
	}

	return ts;
}

#define FLAG_ABSOLUTE 1
#define FLAG_CLOCK_GETTIME 2
#define FLAG_TIMESPEC_GET 4
#define FLAG_CLOCK_NANOSLEEP 8
#define FLAG_NANOSLEEP 16
#define FLAG_THREAD_SLEEP 32
#define FLAG_MONOTONIC 128
#define FLAG_MONOTONIC_RAW 256

clod_once discover_mode_once = CLOD_ONCE_INIT;
int mode = 0;

void discover_mode() {
	#if defined(CLOCK_MONOTONIC) && CLOD_HAVE_CLOCK_GETTIME && CLOD_HAVE_CLOCK_NANOSLEEP && defined(TIMER_ABSTIME)
	do {
		struct timespec time;
		if (clock_gettime(CLOCK_MONOTONIC, &time) != 0)
			break;

		time = add_timespec(time, 10000);
		if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, nullptr) != 0)
			break;

		mode = FLAG_MONOTONIC | FLAG_CLOCK_GETTIME | FLAG_CLOCK_NANOSLEEP | FLAG_ABSOLUTE;
		return;
	} while (0);
	#endif

	#if defined(CLOCK_MONOTONIC) && CLOD_HAVE_CLOCK_GETTIME && CLOD_HAVE_CLOCK_NANOSLEEP
	do {
		struct timespec time;
		if (clock_gettime(CLOCK_MONOTONIC, &time) != 0)
			break;

		time = to_timespec(10000);
		if (clock_nanosleep(CLOCK_MONOTONIC, 0, &time, nullptr) != 0)
			break;

		mode = FLAG_MONOTONIC | FLAG_CLOCK_GETTIME | FLAG_CLOCK_NANOSLEEP;
		return;
	} while (0);
	#endif

	#if defined(CLOCK_MONOTONIC_RAW) && CLOD_HAVE_CLOCK_GETTIME && CLOD_HAVE_CLOCK_NANOSLEEP && defined(TIMER_ABSTIME)
	do {
		struct timespec time;
		if (clock_gettime(CLOCK_MONOTONIC_RAW, &time) != 0)
			break;

		time = add_timespec(time, 10000);
		if (clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &time, nullptr) != 0)
			break;

		mode = FLAG_MONOTONIC_RAW | FLAG_CLOCK_GETTIME | FLAG_CLOCK_NANOSLEEP | FLAG_ABSOLUTE;
		return;
	} while (0);
	#endif

	#if defined(CLOCK_MONOTONIC_RAW) && CLOD_HAVE_CLOCK_GETTIME && CLOD_HAVE_CLOCK_NANOSLEEP
	do {
		struct timespec time;
		if (clock_gettime(CLOCK_MONOTONIC_RAW, &time) != 0)
			break;

		time = to_timespec(10000);
		if (clock_nanosleep(CLOCK_MONOTONIC_RAW, 0, &time, nullptr) != 0)
			break;

		mode = FLAG_MONOTONIC_RAW | FLAG_CLOCK_GETTIME | FLAG_CLOCK_NANOSLEEP;
		return;
	} while (0);
	#endif

	#if defined(CLOCK_MONOTONIC) && CLOD_HAVE_CLOCK_GETTIME && CLOD_HAVE_NANOSLEEP
	do {
		struct timespec time;
		if (clock_gettime(CLOCK_MONOTONIC, &time) != 0)
			break;

		time = to_timespec(10000);
		if (nanosleep(&time, nullptr) != 0)
			break;

		mode = FLAG_MONOTONIC | FLAG_CLOCK_GETTIME | FLAG_NANOSLEEP;
		return;
	} while (0);
	#endif

	#if defined(CLOCK_MONOTONIC_RAW) && CLOD_HAVE_CLOCK_GETTIME && CLOD_HAVE_NANOSLEEP
	do {
		struct timespec time;
		if (clock_gettime(CLOCK_MONOTONIC_RAW, &time) != 0)
			break;

		time = to_timespec(10000);
		if (nanosleep(&time, nullptr) != 0)
			break;

		mode = FLAG_MONOTONIC_RAW | FLAG_CLOCK_GETTIME | FLAG_NANOSLEEP;
		return;
	} while (0);
	#endif

	#if defined(CLOCK_MONOTONIC) && CLOD_HAVE_TIMESPEC_GET && CLOD_HAVE_NANOSLEEP
	do {
		struct timespec time;
		if (timespec_get(&time, CLOCK_MONOTONIC) != CLOCK_MONOTONIC)
			break;

		time = to_timespec(10000);
		if (nanosleep(&time, nullptr) != 0)
			break;

		mode = FLAG_MONOTONIC | FLAG_TIMESPEC_GET | FLAG_NANOSLEEP;
		return;
	} while (0);
	#endif

	#if defined(CLOCK_MONOTONIC) && CLOD_HAVE_TIMESPEC_GET && CLOD_HAVE_STDTHREADS
	do {
		struct timespec time;
		if (timespec_get(&time, CLOCK_MONOTONIC) != CLOCK_MONOTONIC)
			break;

		time = to_timespec(10000);
		if (thrd_sleep(&time, nullptr) != 0)
			break;

		mode = FLAG_MONOTONIC | FLAG_TIMESPEC_GET | FLAG_THREAD_SLEEP;
		return;
	} while (0);
	#endif

	fprintf(stderr, "libclod: no system clock methods worked on this system.");
	exit(EXIT_FAILURE);
}

static int64_t now() {
	CLOD_ONCE(&discover_mode_once)
		discover_mode();

	#if CLOD_HAVE_CLOCK_GETTIME
	if (mode & FLAG_CLOCK_GETTIME) {

		int clock_id = 0;
		#if defined(CLOCK_MONOTONIC)
		if (mode & FLAG_MONOTONIC) clock_id = CLOCK_MONOTONIC;
		#endif
		#if defined(CLOCK_MONOTONIC_RAW)
		if (mode & FLAG_MONOTONIC_RAW) clock_id = CLOCK_MONOTONIC_RAW;
		#endif

		struct timespec time = {0};
		if (clock_gettime(clock_id, &time) == 0)
			return from_timespec(time);
	}
	#endif

	#if CLOD_HAVE_TIMESPEC_GET
	if (mode & FLAG_TIMESPEC_GET) {

		int clock_id = 0;
		#if defined(CLOCK_MONOTONIC)
		if (mode & FLAG_MONOTONIC) clock_id = CLOCK_MONOTONIC;
		#endif
		#if defined(CLOCK_MONOTONIC_RAW)
		if (mode & FLAG_MONOTONIC_RAW) clock_id = CLOCK_MONOTONIC_RAW;
		#endif

		struct timespec time = {0};
		if (timespec_get(&time, clock_id) == clock_id)
			return from_timespec(time);
	}
	#endif

	#if CLOD_DEBUG_THREAD
	fprintf(stderr, "libclod: failed to get system clock.");
	#endif
	return 0;
}

int64_t clod_timer(int64_t *time, const int64_t duration_us) {
	int64_t enter = now();
	if (duration_us <= 0) {
		return enter;
	}

	int64_t end = enter + duration_us;
	if (time) {
		end = *time += duration_us;
		if (end - enter < 0) {
			return enter;
		}
	}

	#if CLOD_HAVE_CLOCK_NANOSLEEP
	if (mode & FLAG_CLOCK_NANOSLEEP) {
		int clock = 0;
		#if defined(CLOCK_MONOTONIC)
		if (mode & FLAG_MONOTONIC) clock = CLOCK_MONOTONIC;
		#endif
		#if defined(CLOCK_MONOTONIC_RAW)
		if (mode & FLAG_MONOTONIC_RAW) clock = CLOCK_MONOTONIC_RAW;
		#endif

		int flags = 0;
		struct timespec wait_ts = to_timespec(end - enter);
		#if defined(TIMER_ABSTIME)
		if (mode & FLAG_ABSOLUTE) {
			flags |= TIMER_ABSTIME;
			wait_ts = to_timespec(end);
		}
		#endif

		struct timespec remaining_ts;
		while (1) {
			int res = clock_nanosleep(clock, flags, &wait_ts, &remaining_ts);
			if (res == 0) {
				return enter;
			}

			if (res != EINTR) {
				#if CLOD_DEBUG_THREAD
				fprintf(stderr, "libclod: clock_nanosleep: %s\n", strerror(res));
				#endif
				if (time) *time -= duration_us;
				return enter;
			}

			if (time) {
				*time -= duration_us;
				return enter;
			}

			if (!(mode & FLAG_ABSOLUTE)) {
				wait_ts = remaining_ts;
			}
		};
	}
	#endif

	#if CLOD_HAVE_NANOSLEEP
	if (mode & FLAG_NANOSLEEP) {
		struct timespec wait_ts = to_timespec(end - enter);
		struct timespec remaining_ts;
		while (1) {
			int res = nanosleep(&wait_ts, &remaining_ts);
			if (res == 0) {
				return enter;
			}

			if (errno != EINTR) {
				#if CLOD_DEBUG_THREAD
				fprintf(stderr, "libclod: nanosleep: %s\n", strerror(errno));
				#endif
				if (time) *time -= duration_us;
				return enter;
			}
			
			if (time) {
				*time -= duration_us;
				return enter;
			}
			
			wait_ts = remaining_ts;
		};
	}
	#endif

	#if CLOD_HAVE_STDTHREADS
	if (mode & FLAG_THREAD_SLEEP) {
		struct timespec wait_ts = to_timespec(end - enter);
		struct timespec remaining_ts;
		while (1) {
			int res = thrd_sleep(&wait_ts, &remaining_ts);
			if (res == 0) {
				return enter;
			}

			if (errno != EINTR) {
				#if CLOD_DEBUG_THREAD
				fprintf(stderr, "libclod: thrd_sleep: %s\n", strerror(errno));
				#endif
				if (time) *time -= duration_us;
				return enter;
			}

			if (time) {
				*time -= duration_us;
				return enter;
			}

			wait_ts = remaining_ts;
		};
	}
	#endif

	#if CLOD_DEBUG_THREAD
	fprintf(stderr, "libclod: internal bug. discovered system clock methods don't exist.");
	#endif
	if (time) *time -= duration_us;
	return enter;
}
