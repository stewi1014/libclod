include_guard(GLOBAL)

find_package(lz4 QUIET)
if (TARGET lz4::lz4)
    if (BUILD_SHARED_LIBS)
        add_library(clod_lz4::lz4_shared ALIAS lz4::lz4)
    endif ()
    add_library(clod_lz4::lz4 ALIAS lz4::lz4)
elseif (BUILD_SHARED_LIBS)
    message(WARNING "Using vendored lz4 for dynamic linking instead of system.
        You should use a system liblz4 instead.")
endif ()

if (BUILD_STATIC_LIBS OR (BUILD_SHARED_LIBS AND NOT TARGET lz4::lz4))
    set(CACHE{LZ4_BUNDLED_MODE} TYPE BOOL FORCE VALUE ON)
    set(CACHE{LZ4_BUILD_CLI} TYPE BOOL FORCE VALUE OFF)
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lz4/build/cmake"
        ${CMAKE_CURRENT_BINARY_DIR}/vendor/lz4/cmake
        EXCLUDE_FROM_ALL
    )

    if (BUILD_STATIC_LIBS AND TARGET lz4_static)
        add_library(clod_lz4::lz4_static ALIAS lz4_static)
    endif ()

    if (BUILD_SHARED_LIBS AND NOT TARGET clod_lz4::lz4_shared AND TARGET lz4_shared)
        add_library(clod_lz4::lz4_shared ALIAS lz4_shared)
    endif ()

    if (NOT TARGET clod_lz4::lz4)
        if (TARGET lz4_static)
            add_library(clod_lz4::lz4 ALIAS lz4_static)
        elseif (TARGET lz4_shared)
            add_library(clod_lz4::lz4 ALIAS lz4_shared)
        endif ()
    endif ()
endif ()
