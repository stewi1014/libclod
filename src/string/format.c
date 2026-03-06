#include <clod/string.h>
#include "clod/debug.h"
#include <stdarg.h>
#include <assert.h>

constexpr unsigned char to_digit[256] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
constexpr unsigned char from_digit[256] = {
	['0'] = 0 , ['1'] = 1 , ['2'] = 2 , ['3'] = 3 , ['4'] = 4 , ['5'] = 5 , ['6'] = 6 , ['7'] = 7 , ['8'] = 8 , ['9'] = 9 ,
	['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15, ['G'] = 16, ['H'] = 17, ['I'] = 18, ['J'] = 19,
	['K'] = 20, ['L'] = 21, ['M'] = 22, ['N'] = 23, ['O'] = 24, ['P'] = 25, ['Q'] = 26, ['R'] = 27, ['S'] = 28, ['T'] = 29,
	['U'] = 30, ['V'] = 31, ['W'] = 32, ['X'] = 33, ['Y'] = 34, ['Z'] = 35, ['a'] = 36, ['b'] = 37, ['c'] = 38, ['d'] = 39,
	['e'] = 40, ['f'] = 41, ['g'] = 42, ['h'] = 43, ['i'] = 44, ['j'] = 45, ['k'] = 46, ['l'] = 47, ['m'] = 48, ['n'] = 49,
	['o'] = 50, ['p'] = 51, ['q'] = 52, ['r'] = 53, ['s'] = 54, ['t'] = 55, ['u'] = 56, ['v'] = 57, ['w'] = 58, ['x'] = 59,
	['y'] = 60, ['z'] = 61, ['@'] = 62, ['_'] = 63,
};

static bool digit_valid(const char digit, const int base) {
	assert(2 <= base && base <= 64);
	unsigned char n = from_digit[(unsigned char)digit];
	if (n >= base) return false;
	if (to_digit[n] != digit) return false;
	return true;
}
static unsigned parse_int(clod_string *src) {
	if (clod_string_peek_char(*src) == '0') return 0;

	unsigned val = 0;
	while (digit_valid(clod_string_peek_char(*src), 10)) {
		val *= 10;
		val += from_digit[(unsigned char)clod_string_get_char(src)];
	}

	return val;
}

enum format_type {
	type_invalid = 0,
	type_int,		  // i/u
	type_long,        // l/ul
	type_long_long,   // ll/ull
	type_int32,       // i32/u32
	type_int64,       // i64/u64
	type_size,        // size/ptrdiff
	type_ptr,         // ptr
	type_double,      // d
	type_long_double, // ld
	type_string,      // s
};

struct format_specifier {
	size_t max_precision;
	enum format_type type;

	unsigned int pad_width: 6;
	unsigned int base: 6;
	unsigned int is_signed: 1;
	unsigned int zero_pad: 1;
	unsigned int capitalise: 1;
	unsigned int have_precision: 1;
};

struct format_specifier parse_format(clod_string *fmt) {
	clod_string parse = *fmt;

	struct format_specifier specifier = {0};

	// Flags
	while (!digit_valid(clod_string_peek_char(parse), 10) || clod_string_peek_char(parse) == '0') {
		switch (clod_string_get_char(&parse)) {
			case '0': specifier.zero_pad = true; break;
			case 'X': specifier.base = 16; specifier.capitalise = true; break;
			case 'x': specifier.base = 16; break;
			case 'D': specifier.base = 10; break;
			case 'O': specifier.base = 8; break;
			case 'B': specifier.base = 2; break;
			default: return specifier;
		}
	}

	// Pad width
	specifier.pad_width = parse_int(&parse);

	// Max precision
	if (clod_string_peek_char(parse) == '.') {
		specifier.max_precision = parse_int(&parse);
		specifier.have_precision = 1;
	} else {
		specifier.max_precision = 0;
		specifier.have_precision = false;
	}

	// length and type
	// checks ordered from long to short type names
	       if (clod_string_has_prefix(&parse, CLOD_STRING_C("ptrdiff"))) {
		specifier.type = type_size;
		specifier.is_signed = true;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("size"))) {
		specifier.type = type_size;
		specifier.is_signed = false;
	} else  if (clod_string_has_prefix(&parse, CLOD_STRING_C("i32"))) {
		specifier.type = type_int32;
	    specifier.is_signed = true;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("u32"))) {
		specifier.type = type_int32;
		specifier.is_signed = false;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("i64"))) {
		specifier.type = type_int64;
		specifier.is_signed = true;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("u64"))) {
		specifier.type = type_int64;
		specifier.is_signed = false;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("ptr"))) {
		specifier.type = type_ptr;
		specifier.is_signed = false;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("ll"))) {
		specifier.type = type_long_long;
		specifier.is_signed = true;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("ull"))) {
		specifier.type = type_long_long;
		specifier.is_signed = false;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("ld"))) {
		specifier.type = type_long_double;
		specifier.is_signed = true;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("l"))) {
		specifier.type = type_long;
		specifier.is_signed = true;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("ul"))) {
		specifier.type = type_long;
		specifier.is_signed = false;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("i"))) {
		specifier.type = type_int;
		specifier.is_signed = true;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("u"))) {
		specifier.type = type_int;
		specifier.is_signed = false;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("d"))) {
		specifier.type = type_double;
		specifier.is_signed = true;
	} else if (clod_string_has_prefix(&parse, CLOD_STRING_C("s"))) {
		specifier.type = type_string;
		specifier.is_signed = false;
	}

	if (specifier.type == type_ptr && specifier.base == 0) {
		specifier.base = 16;
	} else if (specifier.base == 0) {
		specifier.base = 10;
	}

	*fmt = parse;
	return specifier;
}

size_t clod_string_vformat(clod_string *dst, clod_string fmt, va_list args) {
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
		switch (specifier.type) {
			case type_string:
				char *str = va_arg(args, char*);
				if (specifier.have_precision) {
					if (specifier.max_precision) {
						size += clod_string_cat(dst, (clod_string){.array = str, .length = specifier.max_precision });
					} else {
						size += clod_string_cat(dst, (clod_string){ .array = str, .length = va_arg(args, size_t)});
					}
				} else {
					size += clod_string_cat(dst, clod_string_from_cstr(str));
				}
				break;

			// TODO implement formatting for each type.

			default:
				clod_string_put_char(dst, clod_string_get_char(&fmt));
				size++;
		}
	}

	return size;
}
