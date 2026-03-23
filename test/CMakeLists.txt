set(CLOD_TEST_SOURCES
    compression/uniform_behavior.c
    #daemon/task.c
    hash/expected.c
    hash/replace.c
    hash/stream.c
    include/endian.c
    string/basic_format.c
    structures/nbt/benchmark.c
    structures/nbt/parse_level.c
    structures/nbt/print_level.c
    structures/table/set_get_many.c
    structures/table/simple_all_methods.c
    thread/background.c
    thread/benchmark.c
    thread/mutex.c
    thread/spinlock.c
)

create_test_sourcelist(CLOD_TEST_MAIN_SOURCES test_main.c ${CLOD_TEST_SOURCES})

add_executable(clod_test_main ${CLOD_TEST_MAIN_SOURCES})
target_include_directories(clod_test_main PRIVATE . ../src/include)
target_link_libraries(clod_test_main PRIVATE clod)

foreach(test ${CLOD_TEST_SOURCES})
    cmake_path(REMOVE_EXTENSION test)
    add_test (NAME ${test} COMMAND clod_test_main ${test})
endforeach ()
add_test(NAME all COMMAND clod_test_main -A)
