cmake_minimum_required(VERSION 4.0)
project(example-vocoder LANGUAGES C)

add_executable(example-vocoder main.c)

if (NOT TARGET clod::clod_shared)
    find_package(clod REQUIRED)
endif ()
target_link_libraries(example-vocoder PRIVATE clod::clod_shared)
