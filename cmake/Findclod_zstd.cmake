include_guard(GLOBAL)

find_package(zstd QUIET)
if (TARGET zstd::libzstd_shared)
    if (BUILD_SHARED_LIBS)
        add_library(clod_zstd::zstd_shared ALIAS zstd::libzstd_shared)
    endif ()
    add_library(clod_zstd::zstd ALIAS zstd::libzstd_shared)
elseif (BUILD_SHARED_LIBS)
    message(WARNING "Using vendored libzstd for dynamic linking instead of system.
        You should use a system libzstd instead.")
endif ()

if (BUILD_STATIC_LIBS OR (BUILD_SHARED_LIBS AND NOT TARGET zstd::libzstd_shared))
    set(CACHE{ZSTD_BUILD_PROGRAMS} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{ZSTD_BUILD_TESTS} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{ZSTD_LEGACY_SUPPORT} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{ZSTD_MULTITHREAD_SUPPORT} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{ZSTD_BUILD_STATIC} TYPE BOOL FORCE VALUE ${BUILD_STATIC_LIBS})
    set(CACHE{ZSTD_BUILD_SHARED} TYPE BOOL FORCE VALUE ${BUILD_SHARED_LIBS})
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/zstd/build/cmake"
        ${CMAKE_CURRENT_BINARY_DIR}/vendor/zstd
        EXCLUDE_FROM_ALL
    )

    if(BUILD_STATIC_LIBS AND TARGET libzstd_static)
        add_library(clod_zstd::zstd_static ALIAS libzstd_static)
    endif()

    if(BUILD_SHARED_LIBS AND NOT TARGET clod_zstd::zstd_shared AND TARGET libzstd_shared)
        add_library(clod_zstd::zstd_shared ALIAS libzstd_shared)
    endif()

    if (NOT TARGET clod_zstd::zstd)
        if (TARGET libzstd_static)
            add_library(clod_zstd::zstd ALIAS libzstd_static)
        elseif (TARGET libzstd_shared)
            add_library(clod_zstd::zstd ALIAS libzstd_shared)
        endif ()
    endif ()
endif ()
