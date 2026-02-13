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

Building requires CMake >= 4.0 and a C23 compliant toolchain. I test with both GCC and Clang.
All else being equal, GCC and glibc are probably best optimised as that's what I test against the most.
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
Linux is tested.
Some linux-specific optimisations are used if they are available.

#### BSD
BSD variants are untested.
They support everything libclod needs, so the only barrier to support
is getting things plugged in properly if they aren't already.

#### macOS
macOS is untested.
In 14.4 they added a public API for a futex-style feature, which libclod needs.
Before then, projects would use a private and undocumented API for this (yuck!).
Other than that, macOS seems to have a reasonably strong feature set and robust public API.
The only barrier to support is getting things plugged in properly if they aren't already.

#### Windows
Unfortunately, this project highlights Windows's weakness as an operating system and platform.
Not only is Windows unable to share code paths that other operating systems share, requiring
maintaining duplicate code for Windows vs everything else, but functionality ubiquitous among
other systems is often missing in Windows. Due to this lack of functionality, it might not
be possible to implement libclod for Windows without significant feature culling.
To top it off, Windows also lacks a modern C compiler or toolchain. Libclod is written in modern C.

Libclod is attempting to implement some database-like features, and there's a reason why
robust support for Windows by database software is almost unheard of. While it won't be easy,
and I'm far from finished complaining about it, I'm not willing to write Windows off yet.
Hopefully there's a way to work around most of the problems and get libclod working on Windows.
