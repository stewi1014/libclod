target_sources(clod PRIVATE
    atomic.h
    format.c
    format.h
    futex.h
    lock.c
)

add_subdirectory(futex)
