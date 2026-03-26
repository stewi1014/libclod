cmake_minimum_required(VERSION 4.0)
project(example-audio-echo LANGUAGES C)

add_executable(example-audio-echo main.c)

if (NOT TARGET clod::clod_shared)
    find_package(clod REQUIRED)
endif ()
target_link_libraries(example-audio-echo PRIVATE clod::clod_shared)
