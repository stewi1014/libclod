cmake_minimum_required(VERSION 4.0)
project(example-hello-world LANGUAGES C)

add_executable(example-hello-world main.c)

if (NOT TARGET clod::clod_shared)
    find_package(clod REQUIRED)
endif ()
target_link_libraries(example-hello-world PRIVATE clod::clod_shared)
