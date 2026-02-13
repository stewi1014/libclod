target_sources(clod PRIVATE
        compress.c
        compression.c
        decompress.c
)

libclod_test(uniform_behavior)
