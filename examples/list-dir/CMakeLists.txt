cmake_minimum_required(VERSION 4.0)
project(example-list-dir LANGUAGES C)

add_executable(example-list-dir main.c)

if (NOT TARGET clod::clod_shared)
    find_package(clod REQUIRED)
endif ()
target_link_libraries(example-list-dir PRIVATE clod::clod_shared)
