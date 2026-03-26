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

option(CLOD_BUILD_SHARED "Build shared library" ON)
if (CLOD_BUILD_SHARED)
    add_library(clod_shared SHARED)
    add_library(clod::clod_shared ALIAS clod_shared)
    target_link_libraries(clod_shared PRIVATE
        $<BUILD_INTERFACE:clod>
    )
    target_include_directories(clod_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_definitions(clod_shared PRIVATE CLOD_BUILD_SHARED)
    set_target_properties(clod_shared PROPERTIES
        SOVERSION 1
        OUTPUT_NAME clod
    )
    list(APPEND CLOD_INSTALL_TARGETS clod_shared)
endif ()

option(CLOD_BUILD_STATIC "Build static library" ON)
if (CLOD_BUILD_STATIC)
    add_library(clod_static STATIC)
    add_library(clod::clod_static ALIAS clod_static)
    target_link_libraries(clod_static PRIVATE
        $<BUILD_INTERFACE:clod>
    )
    target_include_directories(clod_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
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

option(CLOD_USE_SYSTEM_LIBS "Prefer dynamically linking to system libraries over static linking to vendored libraries." ON)

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

if (TARGET clod_shared)
    find_package(libdeflate QUIET)
    if (CLOD_USE_SYSTEM_LIBS AND TARGET libdeflate::libdeflate_shared)
        target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:libdeflate::libdeflate_shared>)
    else ()
        if (CLOD_USE_SYSTEM_LIBS)
            message(NOTICE "Shared library statically linking vendored libdeflate.")
        endif ()
        include(libdeflate)
        target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:libdeflate_static>)
    endif ()

    find_package(lz4 QUIET)
    if (CLOD_USE_SYSTEM_LIBS AND TARGET lz4::lz4)
        target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:lz4::lz4>)
    else ()
        if (CLOD_USE_SYSTEM_LIBS)
            message(NOTICE "Shared library statically linking vendored liblz4.")
        endif ()
        include(lz4)
        target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:lz4_static>)
    endif ()

    find_package(LibLZMA QUIET)
    if (CLOD_USE_SYSTEM_LIBS AND TARGET LibLZMA::LibLZMA)
        target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:LibLZMA::LibLZMA>)
    else ()
        if (CLOD_USE_SYSTEM_LIBS)
            message(NOTICE "Shared library statically linking vendored liblzma.")
        endif ()
        include(xz)
        target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:liblzma>)
    endif ()

    find_package(zstd QUIET)
    if (CLOD_USE_SYSTEM_LIBS AND TARGET zstd::libzstd_shared)
        target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:zstd::libzstd_shared>)
    else ()
        if (CLOD_USE_SYSTEM_LIBS)
            message(NOTICE "Shared library statically linking vendored libzstd.")
        endif ()
        include(zstd)
        target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:libzstd_static>)
    endif ()

    if (UNIX)
        option(CLOD_USE_ALSA "Use ALSA to enable audio features." ON)
        if (CLOD_USE_ALSA)
            find_package(ALSA REQUIRED)
            target_link_libraries(clod_shared PUBLIC $<BUILD_INTERFACE:ALSA::ALSA>)
        endif ()
    endif ()
endif ()

if (TARGET clod_static)
    include(libdeflate)
    target_link_libraries(clod_static PUBLIC libdeflate_static)
    set_target_properties(libdeflate_static PROPERTIES PUBLIC_HEADER "")
    list(APPEND CLOD_INSTALL_TARGETS libdeflate_static)

    include(lz4)
    target_link_libraries(clod_static PUBLIC lz4_static)
    list(APPEND CLOD_INSTALL_TARGETS lz4_static)

    include(xz)
    target_link_libraries(clod_static PUBLIC liblzma)
    list(APPEND CLOD_INSTALL_TARGETS liblzma)

    include(zstd)
    target_link_libraries(clod_static PUBLIC libzstd_static)
    list(APPEND CLOD_INSTALL_TARGETS libzstd_static)
endif ()

###########
# Install #
###########

install(DIRECTORY include/ DESTINATION include)

if (CLOD_INSTALL_TARGETS)
    install(
        TARGETS ${CLOD_INSTALL_TARGETS}
        EXPORT clod-targets
        INCLUDES DESTINATION include
        LIBRARY DESTINATION lib/clod
        ARCHIVE DESTINATION lib/clod
    )

    install(EXPORT clod-targets
        FILE clod-targets.cmake
        NAMESPACE clod::
        DESTINATION lib/cmake/clod
    )

    list(GET CLOD_INSTALL_TARGETS 0 CLOD_DEFAULT_TARGET)
    install(PACKAGE_INFO clod
        EXPORT clod-targets
        DEFAULT_TARGETS ${CLOD_DEFAULT_TARGET}
    )
endif ()

add_subdirectory(examples)
add_subdirectory(src)
