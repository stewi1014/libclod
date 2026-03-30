#include <clod/string.h>
#include <clod/stream.h>

int main() {
	clod_stream_format(clod_stdout, CLOD_STRING_C("Hello World!\n"));
}
