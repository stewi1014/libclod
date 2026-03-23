cmake_minimum_required(VERSION 4.0)
project(audio-echo LANGUAGES C)

add_executable(audio-echo main.c)
find_package(clod REQUIRED)
target_link_libraries(audio-echo PRIVATE clod::clod)
