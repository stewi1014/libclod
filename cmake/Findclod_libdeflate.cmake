include_guard(GLOBAL)

find_package(libdeflate QUIET)

if (TARGET libdeflate::libdeflate_shared)
    if (BUILD_SHARED_LIBS)
        add_library(clod_libdeflate::libdeflate_shared ALIAS libdeflate::libdeflate_shared)
    endif ()
    add_library(clod_libdeflate::libdeflate ALIAS libdeflate::libdeflate_shared)
else ()
    message(WARNING "Using vendored libdeflate for dynamic linking instead of system.
        You should use a system libdeflate instead.")
endif ()

if (BUILD_STATIC_LIBS OR (BUILD_SHARED_LIBS AND NOT TARGET libdeflate::libdeflate_shared))
    set(CACHE{LIBDEFLATE_BUILD_GZIP} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{LIBDEFLATE_BUILD_TESTS} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{LIBDEFLATE_BUILD_SHARED_LIB} TYPE BOOL FORCE VALUE ${BUILD_SHARED_LIBS})
    set(CACHE{LIBDEFLATE_BUILD_STATIC_LIB} TYPE BOOL FORCE VALUE ${BUILD_STATIC_LIBS})
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/libdeflate"
        ${CMAKE_CURRENT_BINARY_DIR}/vendor/libdeflate
        EXCLUDE_FROM_ALL
    )

    if (BUILD_STATIC_LIBS AND TARGET libdeflate_static)
        add_library(clod_libdeflate::libdeflate_static ALIAS libdeflate_static)
    endif ()

    if (BUILD_SHARED_LIBS AND NOT TARGET clod_libdeflate::libdeflate_shared AND TARGET libdeflate_shared)
        add_library(clod_libdeflate::libdeflate_shared ALIAS libdeflate_shared)
    endif ()

    if (NOT TARGET clod_libdeflate::libdeflate)
        if (TARGET libdeflate_static)
            add_library(clod_libdeflate::libdeflate ALIAS libdeflate_static)
        elseif (TARGET libdeflate_shared)
            add_library(clod_libdeflate::libdeflate ALIAS libdeflate_shared)
        endif ()
    endif ()
endif ()
