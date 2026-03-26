#include <clod/string.h>
#include <clod/stream.h>

int main() {
	struct clod_string buff = CLOD_STRING_NEW(256);
	clod_string_format(&buff, CLOD_STRING_C("Hello World!\n"));
	return clod_stdout->write(clod_stdout, &buff);
}
