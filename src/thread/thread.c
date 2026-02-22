#include <clod/thread.h>
#include "clod_config.h"

#if CLOD_HAVE_PTHREAD_H
#include <pthread.h>
#include <alloca.h>
#include <string.h>

clod_spinlock thread_lock = CLOD_SPINLOCK_INIT;
clod_thread_func *thread_func;

size_t buffer_size;
bool buffer_exists;
char buffer[CLOD_THREAD_BUFFER_MAX];

void *clod_pthread_h_func(void *) {
	clod_thread_func *func = thread_func;

	size_t data_size = buffer_size;
	void *data = buffer_exists ? alloca(buffer_size) : nullptr;
	if (buffer_exists) memcpy(data, buffer, buffer_size);

	clod_spinlock_unlock(&thread_lock);

	func(data, data_size);
	return nullptr;
}

bool clod_thread(clod_thread_func *func, [[maybe_unused]] const char *name, const void *data, size_t data_size) {
	if (data_size > CLOD_THREAD_BUFFER_MAX)
		data_size = CLOD_THREAD_BUFFER_MAX;

	clod_spinlock_lock(&thread_lock);
	thread_func = func;

	buffer_size = data_size;
	buffer_exists = data;
	if (data) memcpy(buffer, data, data_size);

	pthread_t thread;
	int res = pthread_create(&thread, nullptr, clod_pthread_h_func, nullptr);
	if (res != 0) {
		clod_spinlock_unlock(&thread_lock);
		return false;
	}

	#if CLOD_HAVE_PTHREAD_SETNAME
		pthread_setname_np(thread, name);
	#endif

	return true;
}

#elif CLOD_HAVE_THREADS_H
#include <threads.h>
#include <alloca.h>
#include <string.h>

clod_spinlock thread_lock = CLOD_SPINLOCK_INIT;
clod_thread_func *thread_func;

mtx_t mtx;

size_t buffer_size;
bool buffer_exists;
char buffer[CLOD_THREAD_BUFFER_MAX];

int clod_threads_h_func(void *) {
	clod_thread_func *func = thread_func;

	size_t data_size = buffer_size;
	void *data = buffer_exists ? alloca(buffer_size) : nullptr;
	if (buffer_exists) memcpy(data, buffer, buffer_size);

	clod_spinlock_unlock(&thread_lock);

	func(data, data_size);
	return 0;
}

bool clod_thread(clod_thread_func *func, const char *, const void *data, size_t data_size) {
	if (data_size > CLOD_THREAD_BUFFER_MAX)
		data_size = CLOD_THREAD_BUFFER_MAX;

	clod_spinlock_lock(&thread_lock);
	thread_func = func;

	buffer_size = data_size;
	buffer_exists = data;
	if (data) memcpy(buffer, data, data_size);

	thrd_t thread;
	const int res = thrd_create(&thread, clod_threads_h_func, nullptr);
	if (res != thrd_success) {
		clod_spinlock_unlock(&thread_lock);
		return false;
	}

	return true;
}

#else
#error "No threading implementation found"
#endif
