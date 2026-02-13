# libclod
#### [Documentation](https://stewi1014.github.io/libclod/)
#### [Source](https://github.com/stewi1014/libclod)
High performance coordinate->blob storage system and a few other things.

Libclod is licenced under the [GNU Affero General Public License v3.0](./LICENCE.txt) or later.
For alternate licencing contact me.

Libclod is still a work in progress.

## Features

### [Region Storage](https://stewi1014.github.io/libclod/group__region.html)
Libclod's region storage is a high-performance coordinate->blob storage system that uses a novel but
backwards compatible file format to support new features such as data integrity and concurrent access.
It supports concurrent access from multiple threads and processes with almost no blocking even between
reads and writes made to the same file, while ensuring data integrity through program and system crashes.

### [Region Format](https://stewi1014.github.io/libclod/group__region_format.html)
It implements libclod's region storage format according to the [specification](https://stewi1014.github.io/libclod/group__region_format.html#region_format), abstracting
the details of the format itself, including synchronisation, allowing programs to be developed that
correctly interact with the format without needing to understand the nuances of the specification.

### [Compression Wrappers](https://stewi1014.github.io/libclod/group__compression.html)
Libclod wraps some compression libraries to provide a single compress and decompress method
with uniform behaviour across all compression methods it supports.
Most compression methods attempt to be compatible with some existing format.
It is used internally and might be helpful for FFI users who have slow native compression libraries.
Shoutout to libdeflate for being a work of art.

### [NBT Parsing](https://stewi1014.github.io/libclod/group__nbt.html)
The NBT parser is fast and doesn't use any memory.
It doesn't provide an intermediate data structure; I don't believe the
want for an intermediate data structure is borne out of sound reasoning.
It recursively steps through NBT data at approx 6GB/s on my machine.

## Building

Building requires CMake >= 4.0 and a C23 compliant toolchain.
GCC has been specifically optimised for, but CLang also does a fine job and is known to work.
If a C23 compliant toolchain doesn't work with this library, then fixing that is a goal.

```bash
mkdir build
cd build
cmake ..
cmake --build .
ctest .
```

## Dependencies

All dependencies are optional!
They can be enabled or disabled at build time,
thereby omitting the features said dependency provided.
The only exception is the C standard library.

That being said, omitting the majority of compression libraries
will make most file formats that this library interacts with unreadable.
Region files, for example, typically use zlib compression, which requires libdeflate.
~~Vendoring some of these dependencies might be a good idea to implement in future.~~
Vendoring compression libraries **is a very good idea** which I intend to implement.

I reserve the right to use the entire feature set of these dependencies;
using libclod with dependencies that have features intentionally disabled is,
in general, not supported, although many specific cases would be fine.

- libc
- libdeflate `-DUSE_LIBDEFLATE=ON/OFF`
- liblz4 `-DUSE_LIBLZ4=ON/OFF`
- liblzma `-DUSE_LIBLZMA=ON/OFF`
- libzstd `-DUSE_LIBZSTD=ON/OFF`
- libbz2 `-DUSE_LIBBZ2=ON/OFF`
- sqlite3 `-DUSE_SQLITE3=ON/OFF`
- libpq `-DUSE_LIBPQ=ON/OFF`

### Platforms

#### Linux
Linux is fully supported and tested with posix code paths.
Some linux-specific optimisations are enabled.

#### BSD
BSD variants might work out of the box. They support everything I need perfectly, and I've tried to follow documentation.
I don't have a BSD machine to test on, so it's possible I missed some things, and minor changes are required for BSD support.
I'll probably test this myself at some point.

#### macOS
macOS 14.4+ might work out of the box.
They only added a proper futex API in 14.4 which I need.
Some hacks exist (accessing non-public methods) for versions before that, which could possibly be used to support <14.4.
I don't have a macOS machine to test on, and since macOS is unique in forcing developers to
buy macs for development, it's highly unlikely I'll ever be able to test it myself.
It's possible, perhapse likely, I missed some things, and minor changes are required for macOS support.

#### Windows
Windows is the only OS that doesn't play well with the others,
so large sections of posix code paths will need to be rewritten from scratch for the Windows API.
In addition, some fundamental OS features simply don't exist on Windows such as IPC methods,
so it might never be possible to implement some features on Windows.
I have intentionally left space in the codebase for alternative code paths on Windows.
