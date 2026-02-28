#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *copy_stack(void *dst) {
	pid_t tid = gettid();
	char buff[256];
	snprintf(buff, sizeof(buff), "/proc/self/task/%d/maps", tid);
	FILE *file = fopen(buff, "r");
	if (!file) file = fopen("/proc/self/maps", "r");
	if (!file) return nullptr;

	uintptr_t stack_start = 0, stack_end = 0;

	while (fgets(buff, sizeof(buff), file)) {
		if (strstr(buff, "[stack]")) {
			char *end;
			stack_start = strtoul(buff, &end, 16);
			stack_end = strtoul(end + 1, &end, 16);
			break;
		}
	}

	fclose(file);

	memcpy(dst, (void*)stack_start, stack_end - stack_start);
	return (void*)stack_end;
}
