include(CheckSymbolExists)
include(CheckIncludeFile)

if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64")
    set(CLOD_HAVE_X86_64 1)
elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "aarch64")
    set(CLOD_HAVE_ARM64 1)
endif ()

if (CLOD_NATIVE)
    check_symbol_exists(statx "sys/stat.h" CLOD_HAVE_STATX)
    check_symbol_exists(mremap "sys/mman.h" CLOD_HAVE_MREMAP)
endif ()

execute_process(COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CLOD_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(config.h.in config.h)
target_include_directories(clod INTERFACE
    INCLUDE "${CMAKE_CURRENT_BINARY_DIR}"
    INCLUDE .
)

macro(clod_sources)
    target_sources(clod INTERFACE ${ARGV})
endmacro()

add_subdirectory(compression)
add_subdirectory(hash)
add_subdirectory(math)
add_subdirectory(memory)
add_subdirectory(nbt)
#add_subdirectory(region)
#add_subdirectory(region_format)
add_subdirectory(rwseq)
add_subdirectory(stream)
add_subdirectory(string)
add_subdirectory(sys)
add_subdirectory(table)
add_subdirectory(thread)
add_subdirectory(tree)
