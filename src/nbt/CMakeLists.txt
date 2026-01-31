target_sources(clod PRIVATE
    nbt.c
)

libclod_test(parse_level)
libclod_test(print_level)
libclod_test(benchmark)
