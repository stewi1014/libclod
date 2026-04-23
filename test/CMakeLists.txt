set(CLOD_TESTS
    compression/uniform_behavior.c
    #daemon/task.c
    hash/expected.c
    hash/replace.c
    hash/stream.c
    math/fft_freq_complex.c
    math/fft_freq_mag.c
    math/fft_freq_mag_packed.c
    math/taylor_approximations.c
    serialise/integer.c
    string/basic_format.c
    nbt/benchmark.c
    nbt/parse_level.c
    nbt/print_level.c
    table/set_get_many.c
    table/simple_all_methods.c
    thread/background.c
    thread/benchmark.c
    thread/mutex.c
    thread/spinlock.c
)

create_test_sourcelist(CLOD_TEST_SOURCES test_main.c ${CLOD_TESTS})

if (TARGET clod_shared)
    add_executable(clod_shared_test ${CLOD_TEST_SOURCES})
    target_link_libraries(clod_shared_test PRIVATE clod)
    target_link_libraries(clod_shared_test PRIVATE clod_shared)
    target_link_libraries(clod_shared_test PRIVATE m)

    foreach(test ${CLOD_TESTS})
        cmake_path(REMOVE_EXTENSION test)
        add_test (NAME "shared_${test}" COMMAND clod_shared_test ${test})
    endforeach ()
endif ()

if (TARGET clod_static)
    add_executable(clod_static_test ${CLOD_TEST_SOURCES})
    target_link_libraries(clod_static_test PRIVATE clod)
    target_link_libraries(clod_static_test PRIVATE clod_static)
    target_link_libraries(clod_static_test PRIVATE m)

    foreach(test ${CLOD_TESTS})
        cmake_path(REMOVE_EXTENSION test)
        add_test (NAME "static_${test}" COMMAND clod_static_test ${test})
    endforeach ()
endif ()
