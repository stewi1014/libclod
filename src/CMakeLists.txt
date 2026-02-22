include(CheckSymbolExists)
include(CheckIncludeFile)

check_include_file("pthread.h" CLOD_HAVE_PTHREAD_H)
if (CLOD_NATIVE)
    check_symbol_exists(statx "sys/stat.h" CLOD_HAVE_STATX)
    check_symbol_exists(mremap "sys/mman.h" CLOD_HAVE_MREMAP)
    check_symbol_exists(pthread_setname_np "pthread.h" CLOD_HAVE_PTHREAD_SETNAME)
    check_symbol_exists(pthread_rwlockattr_setkind_np "pthread.h" CLOD_HAVE_PTHREAD_RWLOCKATTR_SETKIND)
endif ()
check_include_file("threads.h" CLOD_HAVE_THREADS_H)
check_symbol_exists(sched_yield "sched.h" CLOD_HAVE_SCHED_YIELD)
check_symbol_exists(CLOCK_MONOTONIC "time.h" CLOD_HAVE_CLOCK_MONOTONIC)
check_symbol_exists(CLOCK_MONOTONIC_RAW "time.h" CLOD_HAVE_CLOCK_MONOTONIC_RAW)
check_symbol_exists(clock_gettime "time.h" CLOD_HAVE_CLOCK_GETTIME)
check_symbol_exists(nanosleep "time.h" CLOD_HAVE_NANOSLEEP)
check_symbol_exists(clock_nanosleep "time.h" CLOD_HAVE_CLOCK_NANOSLEEP)

execute_process(COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CLOD_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(clod_config.h.in clod_config.h)
target_include_directories(clod
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    PRIVATE include
)

add_subdirectory(compression)
add_subdirectory(daemon)
add_subdirectory(futex)
add_subdirectory(hash)
add_subdirectory(nbt)
#add_subdirectory(region)
#add_subdirectory(region_format)
add_subdirectory(rwseq)
add_subdirectory(sstr)
add_subdirectory(table)
add_subdirectory(thread)
