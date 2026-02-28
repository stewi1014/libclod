target_sources(clod PRIVATE
    keepalive.c
    keepalive.h
    keepalive_daemon.c
    lock.c
)

set_source_files_properties(
    keepalive_daemon.c PROPERTIES
    COMPILE_OPTIONS "-fno-sanitize=all;-fno-stack-protector;-ffreestanding;-nostdlib;-fno-threadsafe-statics"
)
