# libclod
C library for interacting with NBT, region files, LOD data and other things.

### Public headers

- `compression.h` - Generalised interface for compression algorithms.
- `hash.h` - Hashing functions.
- `nbt.h` - NBT parsing and writing.
- `region.h` - Region file reading and writing.
- `table.h` - Hash table implementation.

### Dependencies

- libdeflate
- liblz4
- libzstd
- liblzma
- sqlite3
- libpq

### Development

The project is written with C23 and Cmake 4.0.

GCC is specifically optimised for, but any C23 compliant toolchain such as Clang should work.
If a C23 compliant toolchain doesn't work with this project, then fixing that is a goal.

### Building

Use CMake as normal.
Those unfamiliar with normal C workflows should note that you're responsible for providing dependencies.

### Platforms

I'm developing on linux.x86_64.
Other architectures and posix-compliant platforms should work with no, or minimal changes.
If you use a different platform, I'd love any help you can offer in supporting it.

At the time of writing;
Any combination of *BSD, Linux, x86_64 and ARM should work perfectly if the system is up to date.
With any luck I've one-shot OSX support. I've tried to have pure posix fallbacks for everything.
Windows is missing some platform-dependent functions, and support will need to be added.
