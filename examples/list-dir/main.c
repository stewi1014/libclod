#include <clod/stream/stream.h>
#include <clod/stream/file.h>
#include <clod/sys/sys.h>

int main() {
	clod_file dir;

	int res = clod_stream_file(&dir, nullptr, "./", CLOD_FILE_DIRECTORY | CLOD_FILE_READ);
	if (res < 0) {
		clod_stream_format(clod_stream_stderr, CLOD_STRING_C("Failed to open directory \"./\": %i\n"), res);
		clod_exit(1);
	}

	struct clod_string buff = CLOD_STRING_NEW(1024);
	while (dir.stream.read(&dir.stream, &buff) == CLOD_ERR_OK) {

		struct clod_dirent *ent = (void*)buff.ptr;
		while (ent) {
			clod_stream_format(clod_stream_stdout, CLOD_STRING_C("type %i, id: %u64, %s\n"), ent->type, (uint64_t)ent->id, ent->name);
			ent = ent->next;
		}

		buff.len = 0;
	}

	dir.stream.close(&dir.stream);
}
