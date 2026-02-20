#include "bitarray.h"
#include "test.h"

int main() {

	for (size_t i = 0; i < 10000; i++) {
		bitarray b;
		b.len = i;
		b.array = malloc(bitarray_array_size(i));
		const auto r = bitarray_len(b);
	}

	bitarray(17) bits = {0};
	constexpr auto r = bitarray_len(bits);
	auto r2 = bitarray_all_unset(bits);

	check("Bits are zero on zero initialisation", bitarray_all_unset(bits));
	bitarray_set(bits, 16);
	check("Getting bit works", bitarray_get(bits, 16));
}