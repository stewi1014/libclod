#include "config.h"
#include "keepalive.h"

#if defined(__linux__)
	#include <unistd.h>
	#include <sys/syscall.h>

	__attribute__((noreturn))
	void __stack_chk_fail(void) {
		syscall(SYS_write, 2, "libclod: stack check failed\n", 28);
		syscall(SYS_exit, 1);
		__builtin_unreachable();
	}
#endif

int keepalive_main(int, char **) {
	return 1;
}
