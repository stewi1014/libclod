target_sources(clod PRIVATE
    mutex.c
    once.c
    spinlock.c
    thread.c
    thread_impl.h
    timer.c
    yield.h
)

option(CLOD_DEBUG_THREAD "Enable or disable extra checks for threading and synchronisation primitives" ${CLOD_DEBUG})

check_symbol_exists(sched_yield "sched.h" CLOD_HAVE_SCHED_YIELD)
check_include_file("linux/sched.h" CLOD_HAVE_LINUX_SCHED)

check_symbol_exists(clock_gettime "time.h" CLOD_HAVE_CLOCK_GETTIME)
check_symbol_exists(timespec_get "time.h" CLOD_HAVE_TIMESPEC_GET)
check_symbol_exists(nanosleep "time.h" CLOD_HAVE_NANOSLEEP)
check_symbol_exists(clock_nanosleep "time.h" CLOD_HAVE_CLOCK_NANOSLEEP)

check_include_file("pthread.h" CLOD_HAVE_PTHREAD)
check_include_file("threads.h" CLOD_HAVE_STDTHREADS)
if (CLOD_HAVE_PTHREAD)
    check_symbol_exists(pthread_setname_np "pthread.h" CLOD_HAVE_PTHREAD_SETNAME)
    check_symbol_exists(pthread_rwlockattr_setkind_np "pthread.h" CLOD_HAVE_PTHREAD_RWLOCKATTR_SETKIND)
endif ()

configure_file(clod_thread_config.h.in clod_thread_config.h)
target_include_directories(clod PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if (CLOD_HAVE_PTHREAD)
    target_sources(clod PRIVATE
        thread_pthread.c
    )
endif ()

if (CLOD_HAVE_STDTHREADS)
    target_sources(clod PRIVATE
        thread_stdthreads.c
    )
endif ()

if (CLOD_HAVE_LINUX_SCHED)
    target_sources(clod PRIVATE
        thread_linux.c
    )
endif ()

add_subdirectory(futex)
