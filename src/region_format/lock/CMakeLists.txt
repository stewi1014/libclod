target_sources(clod PRIVATE
    futex.h
    lock.c
)

add_subdirectory(futex)
