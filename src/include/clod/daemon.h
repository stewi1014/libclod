#ifndef LIBCLOD_DAEMON_H
#define LIBCLOD_DAEMON_H

#include <stdint.h>

/**
 * Add a task for the daemon thread to complete approximately every 200ms.
 * Adding a task with the same id more than once requires the same number of
 * calls to daemon_del to remove.
 * @param[in] id Unique id to identify the task by.
 * @param[in] task_func Function to call every 100ms.
 * If the task returns true, it is removed from the task list.
 * @param[in] user Value to pass to \p task.
 */
void daemon_add(uintptr_t id, bool (*task_func)(void *user), void *user);

/**
 * Remove a task from the daemon's task list.
 * Must be called the same number of times the task was added to entirely remove it.
 * @param[in] id Unique id identifying the task.
 */
void daemon_del(uintptr_t id);

#endif
