#include <clod/thread.h>

#include "test.h"
#include "clod/daemon.h"

volatile int daemon_task_test_value = 0;

bool daemon_task_func(void *user) {
	if (user != (void*)1) return true;
	if (daemon_task_test_value < 0) return true;
	daemon_task_test_value++;
}

int daemon_task(int, char[]) {
	clod_daemon_add(1, daemon_task_func, (void*)1);
	clod_timer(nullptr, 210 * 1000);

	test_check(daemon_task_test_value > 0, "Daemon task should increment number")
		return 1;

	clod_daemon_add(1, daemon_task_func, (void*)2);
	daemon_task_test_value = 0;
	clod_timer(nullptr, 200 * 1000);

	test_check(daemon_task_test_value == 0, "Daemon task should have exited without incrementing number")
		return 1;

	clod_daemon_add(1, daemon_task_func, (void*)2);
	clod_daemon_add(1, daemon_task_func, (void*)2);
	clod_daemon_add(1, daemon_task_func, (void*)2);
	clod_daemon_add(1, daemon_task_func, (void*)1);
	clod_daemon_del(1);
	clod_daemon_del(1);
	clod_daemon_del(1);
	clod_timer(nullptr, 200 * 1000);

	test_check(daemon_task_test_value > 0, "After 4 adds and 3 deletes, the task should still be running")
		return 1;

	clod_daemon_del(1);
	daemon_task_test_value = 0;
	clod_timer(nullptr, 200 * 1000);

	test_check(daemon_task_test_value == 0, "After 4 adds and 4 deletes, the task should be stopped")
		return 1;

	clod_timer(nullptr, 1500 * 1000);

	return 0;
}
