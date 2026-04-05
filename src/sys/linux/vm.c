#include <clod/sys/vm.h>
#include <linux/mman.h>
#include "syscall.h"

size_t clod_vm_page_size() {
	return 4096;
}

void *clod_vm_alloc(const size_t size) {
	const long ret = syscall(__NR_mmap, 0, (long)size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ret < 0 && ret >= -4096) {
		return nullptr;
	}
	return (void*)ret;
}

void clod_vm_free(void *ptr, const size_t size) {
	syscall(__NR_munmap, (long)ptr, (long)size);
}
