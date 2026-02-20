cmake_minimum_required(VERSION 4.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(
    CMAKE_C_STANDARD 23
    CMAKE_C_STANDARD_REQUIRED ON
)

project(libclod LANGUAGES C)
include(CTest)
enable_testing()

add_library(clod SHARED)
set_target_properties(clod PROPERTIES SOVERSION 1)
target_include_directories(clod PUBLIC include)
target_compile_definitions(clod PRIVATE _GNU_SOURCE _FILE_OFFSET_BITS=64)
set_target_properties(clod PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

option(CLOD_REGION_DEBUG "Enables extra checks to ensure region file interaction follows spec" ON)

option(CLOD_NATIVE "Optimize for host architecture" ON)
if(CLOD_NATIVE)
    add_compile_options(-march=native)
endif()

option(CLOD_SANITISERS "Enable sanitisers" OFF)
if(CLOD_SANITISERS)
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(clod PRIVATE "-Wall" "-Wextra" "-Wpedantic")
    target_compile_options(clod PRIVATE "-Wshadow" "-Wconversion" "-Wdouble-promotion" "-Wnull-dereference" "-Werror")
endif()

################
# Dependencies #
################

option(CLOD_USE_LIBDEFLATE "Use libdeflate to enable CLOD_GZIP, CLOD_ZLIB and CLOD_DEFLATE support" ON)
if (CLOD_USE_LIBDEFLATE)
    find_package(libdeflate REQUIRED)
    target_link_libraries(clod PRIVATE libdeflate::libdeflate_shared)
endif ()

option(CLOD_USE_LIBLZ4 "Use liblz4 to enable CLOD_LZ4F support" ON)
if (CLOD_USE_LIBLZ4)
    find_package(lz4 REQUIRED)
    target_link_libraries(clod PRIVATE LZ4::lz4)
endif ()

option(CLOD_USE_LIBLZMA "Use liblzma to enable CLOD_XZ support" ON)
if (CLOD_USE_LIBLZMA)
    find_package(LibLZMA REQUIRED)
    target_link_libraries(clod PRIVATE LibLZMA::LibLZMA)
endif ()

option(CLOD_USE_LIBZSTD "Use libzstd to enable CLOD_ZSTD support" ON)
if (CLOD_USE_LIBZSTD)
    find_package(zstd REQUIRED)
    target_link_libraries(clod PRIVATE zstd::libzstd_shared)
endif ()

option(CLOD_USE_LIBBZ2 "Use libbz2 to enable CLOD_BZIP2 support" ON)
if (CLOD_USE_LIBBZ2)
    find_package(BZip2 REQUIRED)
    target_link_libraries(clod PRIVATE BZip2::BZip2)
endif ()

#############
# Functions #
#############

function(libclod_test name)
    string(REPLACE "/src" "/test" test_dir ${CMAKE_CURRENT_SOURCE_DIR})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}")
    if(rel_path STREQUAL "." OR rel_path STREQUAL "")
        set(test_name "${name}")
    else()
        string(REPLACE "/" "_" path "${rel_path}")
        set(test_name "${path}_${name}")
    endif()

    add_executable(test_${test_name} ${test_dir}/${name}.c)
    target_include_directories(test_${test_name} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/test include)
    target_link_libraries(test_${test_name} PRIVATE clod)
    add_test(NAME ${test_name} COMMAND test_${test_name})
endfunction()

add_subdirectory(src)
