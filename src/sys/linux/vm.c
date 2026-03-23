#include <clod/sys/vm.h>
#include <linux/mman.h>
#include "syscall.h"

size_t clod_vm_page_size() {
	return 4096;
}

void *clod_vm_alloc(size_t size) {
	void *ptr = syscall_mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	long ret = (long)ptr;
	if (ret < 0 && ret >= -4096) {
		return nullptr;
	}

	return ptr;
}

void clod_vm_free(void *ptr, const size_t size) {
	syscall_munmap(ptr, size);
}
