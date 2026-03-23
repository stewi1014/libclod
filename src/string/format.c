#include <clod/string.h>
#include "clod/debug.h"
#include <stdarg.h>
#include <assert.h>

extern bool digit_valid(char digit, int base);

enum format_type {
	type_invalid = 0,
	type_signed_int, type_unsigned_int,             // i/u
	type_signed_long, type_unsigned_long,           // l/ul
	type_signed_long_long, type_unsigned_long_long, // ll/ull
	type_signed_int32, type_unsigned_int32,         // i32/u32
	type_signed_int64, type_unsigned_int64,         // i64/u64
	type_signed_size, type_unsigned_size,           // ptrdiff/size
	type_ptr,                                       // ptr
	type_double,                                    // d
	type_clod_string,                               // str
	type_string,                                    // s
};

struct format_specifier {
	enum format_type type : 5;
	unsigned int max_precision: 12;
	unsigned int pad_width: 6;
	unsigned int base: 6;
	unsigned int zero_pad: 1;
	unsigned int capitalise: 1;
	unsigned int have_precision: 1;
};

struct format_specifier parse_format(struct clod_string *fmt) {
	struct clod_string parse = *fmt;

	struct format_specifier specifier = {0};

	// Flags
	while (1) {
		switch (clod_string_peek_char(parse)) {
			case '0': specifier.zero_pad = true; break;
			case 'X': specifier.base = 16; specifier.capitalise = true; break;
			case 'x': specifier.base = 16; break;
			case 'D': specifier.base = 10; break;
			case 'O': specifier.base = 8; break;
			case 'B': specifier.base = 2; break;
			default: goto read_width;
		}

		clod_string_get_char(&parse);
	}

read_width:
	// Pad width
	specifier.pad_width = clod_string_get_uint(&parse, 10, CLOD_STRING_DIGIT_ALPHABET) & 63;

	// Max precision
	if (clod_string_peek_char(parse) == '.') {
		specifier.max_precision = clod_string_get_uint(&parse, 10, CLOD_STRING_DIGIT_ALPHABET) & 4095;
		specifier.have_precision = 1;
	} else {
		specifier.max_precision = 0;
		specifier.have_precision = false;
	}

	// length and type
	// checks ordered from long to short type names
	       if (clod_string_remove_prefix(&parse, CLOD_STRING_C("ptrdiff"))) {
		specifier.type = type_signed_size;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("size"))) {
		specifier.type = type_unsigned_size;
	} else  if (clod_string_remove_prefix(&parse, CLOD_STRING_C("i32"))) {
		specifier.type = type_signed_int32;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("u32"))) {
		specifier.type = type_unsigned_int32;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("i64"))) {
		specifier.type = type_signed_int64;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("u64"))) {
		specifier.type = type_unsigned_int64;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("ptr"))) {
		specifier.type = type_ptr;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("str"))) {
		specifier.type = type_clod_string;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("ll"))) {
		specifier.type = type_signed_long_long;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("ull"))) {
		specifier.type = type_unsigned_long_long;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("l"))) {
		specifier.type = type_signed_long;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("ul"))) {
		specifier.type = type_unsigned_long;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("i"))) {
		specifier.type = type_signed_int;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("u"))) {
		specifier.type = type_unsigned_int;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("d"))) {
		specifier.type = type_double;
	} else if (clod_string_remove_prefix(&parse, CLOD_STRING_C("s"))) {
		specifier.type = type_string;
	}

	*fmt = parse;
	return specifier;
}

size_t clod_string_format(struct clod_string *dst, struct clod_string fmt, ...) {
	va_list args;
	va_start(args, fmt);
	size_t size = clod_string_vformat(dst, fmt, args);
	va_end(args);
	return size;
}

size_t clod_string_vformat(struct clod_string *dst, struct clod_string fmt, va_list args) {
	size_t size = 0;

	while (clod_string_peek_char(fmt)) {
		char c = clod_string_get_char(&fmt);
		if (c != '%') {
			clod_string_put_char(dst, c);
			size++;
			continue;
		}

		if (clod_string_peek_char(fmt) == '%') {
			clod_string_put_char(dst, clod_string_get_char(&fmt));
			size++;
			continue;
		}

		struct format_specifier specifier = parse_format(&fmt);
		if (specifier.base == 0) {
			if (specifier.type == type_ptr) {
				specifier.base = 16;
			} else {
				specifier.base = 10;
			}
		}

		struct clod_string digit_alphabet = specifier.capitalise ? CLOD_STRING_DIGIT_ALPHABET_CAPS : CLOD_STRING_DIGIT_ALPHABET;

		switch (specifier.type) {
			case type_signed_int:
				size += clod_string_put_int(dst, va_arg(args, signed int),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_unsigned_int:
				size += clod_string_put_uint(dst, va_arg(args, unsigned int),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_signed_long:
				size += clod_string_put_int(dst, va_arg(args, signed long),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_unsigned_long:
				size += clod_string_put_uint(dst, va_arg(args, unsigned long),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_signed_long_long:
				size += clod_string_put_int(dst, va_arg(args, signed long long),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_unsigned_long_long:
				size += clod_string_put_uint(dst, va_arg(args, unsigned long long),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_signed_int32:
				size += clod_string_put_int(dst, va_arg(args, int32_t),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_unsigned_int32:
				size += clod_string_put_uint(dst, va_arg(args, uint32_t),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_signed_int64:
				size += clod_string_put_int(dst, va_arg(args, int64_t),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_unsigned_int64:
				size += clod_string_put_uint(dst, va_arg(args, uint64_t),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_signed_size:
				size += clod_string_put_int(dst, va_arg(args, ptrdiff_t),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_unsigned_size:
				size += clod_string_put_uint(dst, va_arg(args, size_t),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_ptr:
				clod_string_put_char(dst, '0');
				clod_string_put_char(dst, 'x');
				size += 2 + clod_string_put_uint(dst, (uintptr_t)va_arg(args, void*),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_double:
				size += clod_string_put_double(dst, va_arg(args, double),
					digit_alphabet, specifier.base, specifier.pad_width, (unsigned char)specifier.max_precision);
				break;
			case type_clod_string:
				size += clod_string_cat(dst, va_arg(args, struct clod_string));
				break;
			case type_string:
				size += clod_string_cat(dst, clod_string_from_cstr(va_arg(args, const char *)));
				break;
			default:
				clod_string_put_char(dst, clod_string_get_char(&fmt));
				size++;
		}
	}

	return size;
}
