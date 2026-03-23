cmake_minimum_required(VERSION 4.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(
    CMAKE_C_STANDARD 23
    CMAKE_C_STANDARD_REQUIRED ON
    CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64
)

project(libclod LANGUAGES C)

include(CTest)
enable_testing()

###########
# Libclod #
###########

add_library(clod SHARED)
set_target_properties(clod PROPERTIES SOVERSION 1)
target_include_directories(clod PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(clod PROPERTIES C_VISIBILITY_PRESET hidden)
target_compile_definitions(clod PRIVATE CLOD_BUILD)
add_library(clod::clod ALIAS clod)

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
    target_compile_options(clod PRIVATE -march=native)
    set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    target_compile_definitions(clod PRIVATE _GNU_SOURCE)
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

option(CLOD_USE_ALSA "Use alsa to enable audio features" ON)
if (CLOD_USE_ALSA)
    find_package(ALSA REQUIRED)
    target_link_libraries(clod PRIVATE ALSA::ALSA)
endif ()

###########
# Install #
###########

install(TARGETS clod
        EXPORT clod-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT clod-targets
        FILE clod-targets.cmake
        NAMESPACE clod::
        DESTINATION lib/cmake/clod
)

#add_subdirectory(examples)
add_subdirectory(src)
add_subdirectory(test)

