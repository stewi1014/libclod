cmake_minimum_required(VERSION 4.2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(
    CMAKE_C_STANDARD 23
    CMAKE_C_STANDARD_REQUIRED ON
    CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64
    CMAKE_POSITION_INDEPENDENT_CODE ON
)

project(libclod LANGUAGES C)
include(CTest)
enable_testing()

###########
# Libclod #
###########

add_library(clod INTERFACE)
target_include_directories(clod INTERFACE include)
set_target_properties(clod PROPERTIES C_VISIBILITY_PRESET hidden)

set(CLOD_INSTALL_TARGETS)

option(BUILD_SHARED_LIBS "Build shared library" ON)
if (BUILD_SHARED_LIBS)
    add_library(clod_shared SHARED)
    target_link_libraries(clod_shared PRIVATE clod)
    target_compile_definitions(clod_shared PRIVATE CLOD_BUILD_SHARED)
    set_target_properties(clod_shared PROPERTIES
        SOVERSION 1
        OUTPUT_NAME clod
    )
    list(APPEND CLOD_INSTALL_TARGETS clod_shared)
endif ()

option(BUILD_STATIC_LIBS "Build static library" ON)
if (BUILD_STATIC_LIBS)
    add_library(clod_static STATIC)
    target_link_libraries(clod_static PRIVATE clod)
    target_compile_definitions(clod_static PRIVATE CLOD_BUILD_STATIC)
    if (NOT WIN32)
        set_target_properties(clod_static PROPERTIES OUTPUT_NAME clod)
    endif ()
    list(APPEND CLOD_INSTALL_TARGETS clod_static)
endif ()

option(BUILD_TESTING "Build tests" ON)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

#########################
# Configuration Options #
#########################

option(CLOD_DEBUG "Enables extra checks aimed at debugging. Unspecified debug options inherit this setting." ON)
option(CLOD_RWSEQ_DEBUG "Enables extra checks to ensure rwseq usage is normal." ${CLOD_DEBUG})
option(CLOD_REGION_DEBUG "Enables extra checks to ensure region file interaction follows spec." ${CLOD_DEBUG})
option(CLOD_MEMORY_DEBUG "Enables extra memory consistency checks aimed at debugging." ${CLOD_DEBUG})
option(CLOD_MEMORY_CHECK "Enables memory consistency checks aimed at release builds." ON)

option(CLOD_NATIVE "Optimise for host architecture and use nonportable platform features" ON)
if(CLOD_NATIVE)
    set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    target_compile_options(clod INTERFACE -march=native)
    target_compile_definitions(clod INTERFACE _GNU_SOURCE)
endif()

option(CLOD_SANITISERS "Enable sanitisers" OFF)
if(CLOD_SANITISERS)
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(clod INTERFACE "-Wall" "-Wextra" "-Wpedantic" "-Wshadow" "-Wconversion" "-Wdouble-promotion" "-Wnull-dereference" "-Werror")
endif()

################
# Dependencies #
################

option(CLOD_USE_LIBDEFLATE "Use libdeflate to enable CLOD_GZIP, CLOD_ZLIB and CLOD_DEFLATE support" ON)
if (CLOD_USE_LIBDEFLATE)
    find_package(clod_libdeflate REQUIRED)
endif()

option(CLOD_USE_LIBLZ4 "Use liblz4 to enable CLOD_LZ4F support" ON)
if (CLOD_USE_LIBLZ4)
    find_package(clod_lz4 REQUIRED)
endif()

option(CLOD_USE_LIBLZMA "Use liblzma to enable CLOD_XZ support" ON)
if (CLOD_USE_LIBLZMA)
    find_package(clod_lzma REQUIRED)
endif()

option(CLOD_USE_LIBZSTD "Use libzstd to enable CLOD_ZSTD support" ON)
if (CLOD_USE_LIBZSTD)
    find_package(clod_zstd REQUIRED)
endif ()

if (UNIX)
    option(CLOD_USE_ALSA "Use alsa to enable audio features" ON)
    if (CLOD_USE_ALSA)
        find_package(ALSA REQUIRED)
    endif()
endif ()

if (BUILD_SHARED_LIBS)
    if (CLOD_USE_LIBDEFLATE)
        target_include_directories(clod_shared PRIVATE )
        target_link_libraries(clod_shared PRIVATE clod_libdeflate::libdeflate)
    endif()

    if (CLOD_USE_LIBLZ4)
        target_link_libraries(clod_shared PRIVATE clod_lz4::lz4)
    endif()

    if (CLOD_USE_LIBLZMA)
        target_link_libraries(clod_shared PRIVATE clod_lzma::lzma)
    endif()

    if (CLOD_USE_LIBZSTD)
        target_link_libraries(clod_shared PRIVATE clod_zstd::zstd)
    endif ()

    if (CLOD_USE_ALSA)
        target_link_libraries(clod_shared PRIVATE ALSA::ALSA)
    endif ()
endif()

if (BUILD_STATIC_LIBS)
    if (CLOD_USE_LIBDEFLATE)
        target_link_libraries(clod_static PUBLIC clod_libdeflate::libdeflate_static)
    endif()

    if (CLOD_USE_LIBLZ4)
        target_link_libraries(clod_static PUBLIC clod_lz4::lz4_static)
    endif()

    if (CLOD_USE_LIBLZMA)
        target_link_libraries(clod_static PUBLIC clod_lzma::lzma_static)
    endif()

    if (CLOD_USE_LIBZSTD)
        target_link_libraries(clod_static PUBLIC clod_zstd::zstd_static)
    endif ()

    if (CLOD_USE_ALSA)
        target_link_libraries(clod_static PUBLIC ALSA::ALSA)
    endif ()
endif ()

###########
# Install #
###########

install(
    TARGETS ${CLOD_INSTALL_TARGETS}
    EXPORT clod-targets
    LIBRARY DESTINATION lib/clod
    ARCHIVE DESTINATION lib/clod
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

#add_subdirectory(examples)
add_subdirectory(src)
