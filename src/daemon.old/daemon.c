#include "clod/daemon.h"
#include <assert.h>
#include <stdio.h>
#include <clod/table.h>
#include <clod/thread.h>
#include <stdlib.h>

clod_spinlock daemon_lock = CLOD_SPINLOCK_INIT;
struct clod_table *daemon_table = nullptr;

struct task {
	uintptr_t id;
	size_t counter;
	bool (*task_func)(void *user);
	void *user;
};

#define PERIOD_US (200 * 1000)

int daemon_run(int, char **) {
	// inherit the lock.
	assert(!daemon_table);
	daemon_table = clod_table_create(nullptr);

	int empty_loops = 0;
	int64_t time = clod_timer(nullptr, 0);

	while (empty_loops++ < 5) {
		clod_spinlock_unlock(&daemon_lock);
		int64_t current = clod_timer(&time, PERIOD_US);
		clod_spinlock_lock(&daemon_lock);
		if (current > time) time = current;

		struct clod_table_iter iter = {0};
		while (clod_table_iter(daemon_table, &iter)) {
			struct task *task = iter.element;

			clod_spinlock_unlock(&daemon_lock);
			bool res = task->task_func(task->user);
			clod_spinlock_lock(&daemon_lock);

			if (res) free(clod_table_del(daemon_table, iter.element, iter.key_size));
		}

		if (clod_table_len(daemon_table) > 0)
			empty_loops = 0;
	}

	clod_table_destroy(daemon_table);
	daemon_table = nullptr;
	clod_spinlock_unlock(&daemon_lock);
	return 0;
}

void clod_daemon_add(const uintptr_t id, bool (*task_func)(void *user), void *user) {
	clod_spinlock_lock(&daemon_lock);

	if (!daemon_table) {
		struct clod_process_opts proc_opts = {
			.type = CLOD_DAEMON,
			.main = daemon_run
		};

		bool res = clod_process_start(&proc_opts, nullptr);
		if (!res) {
			fprintf(stderr, "Failed to start libclod daemon");
			clod_spinlock_unlock(&daemon_lock);
			return;
		}

		// thread inherits the lock.
		// get it again.
		clod_spinlock_lock(&daemon_lock);
	}

	assert(daemon_table);
	struct task *task = clod_table_get(daemon_table, &id, sizeof(id));
	if (task) {
		task->counter++;
		task->task_func = task_func;
		task->user = user;
	} else {
		task = malloc(sizeof(struct task));
		task->id = id;
		task->counter = 1;
		task->task_func = task_func;
		task->user = user;
		clod_table_add(daemon_table, task, sizeof(task->id), nullptr);
	}

	clod_spinlock_unlock(&daemon_lock);
}

void clod_daemon_del(const uintptr_t id) {
	clod_spinlock_lock(&daemon_lock);

	if (daemon_table != nullptr) {
		struct task *task = clod_table_get(daemon_table, &id, sizeof(id));
		if (task) {
			task->counter--;
			if (task->counter == 0)
				free(clod_table_del(daemon_table, &id, sizeof(id)));
		}
	}

	clod_spinlock_unlock(&daemon_lock);
}
