#include "debug.h"
#include <clod/thread.h>

int benchmark_test_thread_main(int, char **) {
	return 0;
}

int benchmark_thread_configuration(struct clod_process_opts *opts, int64_t end) {
	int count = 0;
	while (clod_timer(nullptr, 0) < end) {
		clod_process process;
		auto res = clod_process_start(opts, &process);
		assert_fatal(CLOD_TEST, res == CLOD_PROCESS_OK, "Should be able to create thread");

		res = clod_process_join(process);
		assert_fatal(CLOD_TEST, res == CLOD_PROCESS_OK, "Should be able to join thread");
		count++;
	}

	return count;
}

#define PERIOD (200 * 1000)

int thread_benchmark(int, char[]) {
	int thread_count = benchmark_thread_configuration(
		&(struct clod_process_opts){ .main = benchmark_test_thread_main, .type = CLOD_DAEMON},
		clod_timer(nullptr, 0) + PERIOD
	);
	debug(CLOD_TEST, "Created %i CLOD_DAEMON in %i ms\n", thread_count, PERIOD / 1000);

	thread_count = benchmark_thread_configuration(
		&(struct clod_process_opts){ .main = benchmark_test_thread_main, .type = CLOD_THREAD},
		clod_timer(nullptr, 0) + PERIOD
	);
	debug(CLOD_TEST, "Created %i CLOD_THREAD in %i ms\n", thread_count, PERIOD / 1000);

	return 0;
}
