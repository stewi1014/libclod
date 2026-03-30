cmake_minimum_required(VERSION 3.10)
project(midsommar LANGUAGES C)

add_executable(midsommar main.c)

find_package(clod REQUIRED)
target_link_libraries(midsommar PRIVATE clod::clod_shared)
