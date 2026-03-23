
#include <clod/string.h>

int string_basic_format() {
	struct clod_string str = CLOD_STRING_NEW(1024);
	clod_string_format(
		&str,
		CLOD_STRING_C("int: %i, unsigned: %u, u64: %Xu64, i32: %xi32, str: %str, ptr: %ptr"),
		-123456789, 123456789, ~UINT64_C(0), -15, CLOD_STRING_C("hello"), (void*)0x1a2b2c2d2e2f6
	);

	struct clod_string want = CLOD_STRING_C("int: -123456789, unsigned: 123456789, u64: FFFFFFFFFFFFFFFF, i32: -f, str: hello, ptr: 0x1a2b2c2d2e2f6");
	return clod_string_cmp(str, want);
}
