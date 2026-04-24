#include <clod/string.h>
#include <clod/stream/stream.h>

int main() {
	clod_stream_format(clod_stream_stdout, CLOD_STRING_C("Hello World!\n"));
}
