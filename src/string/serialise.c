#include <clod/string.h>

static bool base_valid(const int base, const struct clod_string alphabet) {
	return 2 <= base && base <= 64 && base <= alphabet.len;
}

static int digit_count(uintmax_t value, const int base) {
	int count = 0;
	do {
		value /= (uintmax_t)base;
		count++;
	} while (value > 0);
	return count;
}

size_t clod_string_put_int(struct clod_string *dst, intmax_t val,
	const struct clod_string alphabet, const unsigned char base, const unsigned char min_digits, const unsigned char max_digits) {
	if (!base_valid(base, alphabet)) {
		return 0;
	}

	if (val < 0) {
		clod_string_put_char(dst, '-');
		val = -val;
	} else {
		clod_string_put_char(dst, '+');
	}

	return clod_string_put_uint(dst, (uintmax_t)val, alphabet, base, min_digits, max_digits) + 1;
}
intmax_t clod_string_get_int(struct clod_string *str, const unsigned char base, const struct clod_string alphabet) {
	if (!str->ptr || str->len < 2 || !base_valid(base, alphabet)) {
		return 0;
	}

	if (str->ptr[0] != '+' && str->ptr[0] != '-') {
		return 0;
	}

	if (!clod_string_find(alphabet, str->ptr[0], 1).ptr) {
		return 0;
	}

	if (str->ptr[0] == '+') {
		return (intmax_t)clod_string_get_uint(str, base, alphabet);
	} else {
		return -(intmax_t)clod_string_get_uint(str, base, alphabet);
	}
}
size_t clod_string_put_uint(struct clod_string *dst, uintmax_t val,
	const struct clod_string alphabet, const unsigned char base, const unsigned char min_digits, const unsigned char max_digits) {
	if (!dst->ptr || !base_valid(base, alphabet)) {
		return 0;
	}

	int size = digit_count(val, base);
	if (max_digits && size > max_digits) size = max_digits;
	if (size < min_digits) size = min_digits;

	for (int i = size - 1; i >= 0; i--) {
		if (dst->cap - dst->len - i > 0) {
			dst->ptr[dst->len + i] = alphabet.ptr[val % (uintmax_t)base];
		}
		 val /= (uintmax_t)base;
	}

	dst->len += size;
	return (size_t)size;
}
uintmax_t clod_string_get_uint(struct clod_string *str, const unsigned char base, const struct clod_string alphabet) {
	if (!str->ptr || str->len < 1 || !base_valid(base, alphabet)) {
		return 0;
	}

	uintmax_t result = 0;
	while (str->len > 0) {
		struct clod_string digit = clod_string_find(alphabet, str->ptr[0], 1);
		if (!digit.ptr || digit.ptr - alphabet.ptr >= base) break;

		result *= (uintmax_t)base;
		result += (uintmax_t)(digit.ptr - alphabet.ptr);
		str->ptr++;
		str->len--;
	}

	return result;
}
size_t clod_string_put_double(struct clod_string *dst, double,
	const struct clod_string, unsigned char, unsigned char, unsigned char) {
	return clod_string_cat(dst, CLOD_STRING_C("<DOUBLE NOT IMPLEMENTED>"));
}
double clod_string_get_double(struct clod_string *, unsigned char, const struct clod_string) {
	return 0;
}
