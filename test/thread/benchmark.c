#include "test.h"
#include <clod/thread.h>

int benchmark_test_thread_main(int argc, char **argv) {
	return 0;
}

int benchmark_thread_configuration(struct clod_process_opts *opts, int64_t end) {
	int count = 0;
	while (clod_timer(nullptr, 0) < end) {
		clod_process process;
		auto res = clod_process_start(opts, &process);
		test_check(res == CLOD_PROCESS_OK, "Should be able to create thread")
			return 0;

		res = clod_process_join(process);
		test_check(res == CLOD_PROCESS_OK, "Should be able to join thread")
			return 0;
		
		count++;
	}

	return count;
}

#define PERIOD (1000 * 1000)

int thread_benchmark(int, char[]) {
	int thread_count = benchmark_thread_configuration(
		&(struct clod_process_opts){ .main = benchmark_test_thread_main, .type = CLOD_DAEMON},
		clod_timer(nullptr, 0) + PERIOD
	);
	fprintf(stdout, "Created %d CLOD_DAEMON in %d ms\n", thread_count, PERIOD / 1000);

	thread_count = benchmark_thread_configuration(
		&(struct clod_process_opts){ .main = benchmark_test_thread_main, .type = CLOD_DAEMON},
		clod_timer(nullptr, 0) + PERIOD
	);
	fprintf(stdout, "Created %d CLOD_THREAD in %d ms\n", thread_count, PERIOD / 1000);

	return 0;
}
