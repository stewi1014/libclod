include_guard(GLOBAL)

find_package(LibLZMA QUIET)
if (TARGET LibLZMA::LibLZMA)
    if (BUILD_SHARED_LIBS)
        add_library(clod_lzma::lzma_shared ALIAS LibLZMA::LibLZMA)
    endif ()
    add_library(clod_lzma::lzma ALIAS LibLZMA::LibLZMA)
elseif (BUILD_STATIC_LIBS AND BUILD_DYNAMIC_LIBS)
    message(FATAL_ERROR "liblzma does not support building shared and static libraries at the same time,
        shared system library is missing, and both BUILD_SHARED_LIBS and BUILD_STATIC_LIBS are on.")
elseif (BUILD_SHARED_LIBS)
    message(WARNING "Using vendored liblzma for dynamic linking instead of system
        You should use a system liblzma instead.")
endif ()

if (BUILD_STATIC_LIBS OR (BUILD_SHARED_LIBS AND NOT TARGET LibLZMA::LibLZMA))
    set(CACHE{XZ_DOC} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{XZ_NLS} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{XZ_THREADS} TYPE STRING FORCE VALUE "no")
    set(CACHE{XZ_TOOL_LZMADEC} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{XZ_TOOL_LZMAINFO} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{XZ_TOOL_SCRIPTS} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{XZ_TOOL_SYMLINKS} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{XZ_TOOL_SYMLINKS_LZMA} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{XZ_TOOL_XZ} TYPE BOOL FORCE VALUE OFF)
    set(CACHE{XZ_TOOL_XZDEC} TYPE BOOL FORCE VALUE OFF)

    set(CACHE{BUILD_TESTING_SAVED} TYPE BOOL FORCE VALUE ${BUILD_TESTING})
    set(CACHE{BUILD_TESTING} TYPE BOOL FORCE VALUE OFF)

    set(CACHE{BUILD_SHARED_LIBS_SAVED} TYPE BOOL FORCE VALUE ${BUILD_SHARED_LIBS})
    if (BUILD_STATIC_LIBS)
        set(CACHE{BUILD_SHARED_LIBS} TYPE BOOL FORCE VALUE OFF)
    endif ()

    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/xz"
        ${CMAKE_CURRENT_BINARY_DIR}/vendor/xz
        EXCLUDE_FROM_ALL
    )

    if (BUILD_STATIC_LIBS AND TARGET liblzma)
        add_library(clod_lzma::lzma_static ALIAS liblzma)
    endif ()

    if (BUILD_SHARED_LIBS AND NOT TARGET clod_lzma::lzma_shared AND TARGET liblzma)
        add_library(clod_lzma::lzma_shared ALIAS liblzma)
    endif ()

    if (NOT TARGET clod_lzma::lzma AND TARGET liblzma)
        add_library(clod_lzma::lzma ALIAS liblzma)
    endif ()

    set(BUILD_TESTING ${BUILD_TESTING_SAVED})
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVED})
endif ()
