# libclod
C library for interacting with NBT data, region files, LOD data, data storage and other things.

Libclod has came a long way since I started working on it a few months ago.
A lot of core infrastructure is now in place,
and I've solidified some core data formats the library is using.
There is still a decent amount of functionality from my original proof of concept that
remains to be documented and polished into a good public API, and I won't be rushing any of it.

## Dependencies

- libdeflate
- liblz4
- libzstd
- liblzma
- sqlite3
- libpq

## Building

Use cmake as usual.
`mkdir build && cd build &$ cmake .. && make`

## Features

### [Compression wrappers](https://stewi1014.github.io/libclod/group__compression.html)
Libclod wraps some compression libraries to provide a single uniform interface across all compression algorithms.
It might be helpful for FFI users who have slow native compression libraries.
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
crash recovery, effective concurrency, strong data validation, and more.

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

## Public headers

- `compression.h` - Generalised interface for compression algorithms.
- `hash.h` - Hashing functions.
- `nbt.h` - NBT parsing and writing.
- `region.h` - Region file reading and writing.
- `table.h` - Hash table implementation.

## Development

The project is written with C23 and Cmake 4.0.

GCC is specifically optimised for, but any C23 compliant toolchain such as Clang should work.
If a C23 compliant toolchain doesn't work with this project, then fixing that is a goal.

### Building

Use CMake as normal.
Those unfamiliar with normal C workflows should note that you're responsible for providing dependencies.
Some dependencies are optional and can be disabled at configure time.

### Platforms

I'm developing on linux.x86_64.
Other architectures and posix-compliant platforms should work with no, or minimal changes.
If you use a different platform, I'd love any help you can offer in supporting it.

At the time of writing;
Any combination of *BSD, Linux, x86_64, and ARM should work perfectly if the system is up to date.
With any luck I've one-shot OSX support. I've tried to have pure posix fallbacks for everything.
Windows is missing some platform-dependent functions, and support will need to be added.
