target_sources(clod PRIVATE
    crc.c
    crc_tables.h
    siphash.c
)

libclod_test(expected)
libclod_test(stream)
libclod_test(replace)
