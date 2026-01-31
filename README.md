# libclod
C library for interacting with NBT data, region files, LOD data, data storage and other things.

Libclod is licenced under the [GNU Affero General Public License v3.0](./LICENCE.txt) or later.
For alternate licencing contact me.

Libclod is still a work in progress.

## Dependencies

All dependencies are optional!
They can be enabled or disabled at build time,
thereby omitting the features said dependency provided.
The only exception is the C standard library.

That being said, omitting the majority of compression libraries
will make most file formats that this library interacts with unreadable.
Region files, for example, typically use zlib compression, which requires libdeflate.
Vendoring some of these dependencies might be a good idea to implement in future.

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
Some linux-specific optimisations are enabled when possible.

#### BSD
BSD variants should hopefully work out of the box with posix code paths.
It's possible I missed some things, and minor changes are required for BSD support.

#### macOS
macOS should hopefully work out of the box with posix code paths.
It's possible I missed some things, and minor changes are required for macOS support.

#### Windows
Windows is the only OS that doesn't play well with others,
so platform-specific code sections will need to be rewritten from scratch for the Windows API.
I have intentionally left space in the codebase for this,
but don't have a Windows development environment to work on them.

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

## Public headers

- `clod/compression.h` - Generalised interface for compression algorithms.
- `clod/hash.h` - Hashing functions.
- `clod/nbt.h` - NBT parsing and writing.
- `clod/region.h` - Region storage.
- `clod/table.h` - Hash table implementation.

Some standalone headers are also included for things such as
sized-strings, vector math and big-endian encoding.

## Features

### [Compression wrappers](https://stewi1014.github.io/libclod/group__compression.html)
Libclod wraps some compression libraries to provide a single compress and decompress method
with uniform behavior across all compression methods it supports.
Most compression methods attempt to be compatible with some existing format.
It is used internally and might be helpful for FFI users who have slow native compression libraries.
Shoutout to libdeflate for being a work of art.

### [NBT parsing](https://stewi1014.github.io/libclod/group__nbt.html)
The NBT parser is fast and doesn't use any memory.
It doesn't have an intermediate data structure.
It recursively steps through NBT data at approx 6GB/s on my machine.

### [Region storage](https://stewi1014.github.io/libclod/group__region.html)
This is not a region file parser,
although it might be a good idea to expose a public interface for that in the future.
Instead, libclod's region storage implements the whole storage system,
providing a fast and memory-efficient coordinate->blob file-backed storage database
that leverages low-level OS features to optimise access.

It extends the vanilla implementation with a number of new features,
including support for more compression algorithms, up to 10 dimension coordinates,
crash recovery, effective concurrency, strong data validation and more.

These new features are built on top of a revised file format that is (optionally) backwards compatible
with vanilla region files and supports future extensibility with dynamic storage in the header.

Multithreaded usage is supported, including from coroutines, and is well optimised.
However, multiple processes writing at the same time is not supported.
That might seem obvious, but I don't feel like it's an unreasonable feature to ask for.
Libclod is already aiming to implement features that make it look like a database;
it's not a stretch to ask for one more database-like feature.
I would have implemented this already; POSIX robust shared mutexes are an obvious choice,
but macOS's shared mutexes aren't robust and Windows doesn't even have shared mutexes.
I want to know what kind of architecture is needed for whatever features Windows and macOS might have for this.
Or, complete sufficient research that I'm satisfied in saying I can't ever support macOS or Windows,
and architect the feature around Linux and BSD exclusively.
