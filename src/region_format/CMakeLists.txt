target_sources(clod PRIVATE
    atomic.h
    format.c
    format.h
    futex.h
    lock.c
)

add_subdirectory(futex)

option(DEBUG_REGION_ACCESS "Enables extra checks to ensure region file interaction follows spec" ON)
