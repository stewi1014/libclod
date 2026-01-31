target_sources(clod PRIVATE
        compress.c
        compression.c
        decompress.c
)

option(USE_LIBDEFLATE "Use libdeflate to enable CLOD_GZIP, CLOD_ZLIB and CLOD_DEFLATE support" ON)
if (USE_LIBDEFLATE)
    find_package(libdeflate REQUIRED)
    target_link_libraries(clod PRIVATE libdeflate::libdeflate_shared)
    set(HAVE_LIBDEFLATE 1)
endif ()

option(USE_LIBLZ4 "Use liblz4 to enable CLOD_LZ4F support" ON)
if (USE_LIBLZ4)
    find_package(lz4 REQUIRED)
    target_link_libraries(clod PRIVATE LZ4::lz4)
    set(HAVE_LIBLZ4 1)
endif ()

option(USE_LIBLZMA "Use liblzma to enable CLOD_XZ support" ON)
if (USE_LIBLZMA)
    find_package(LibLZMA REQUIRED)
    target_link_libraries(clod PRIVATE LibLZMA::LibLZMA)
    set(HAVE_LIBLZMA 1)
endif ()

option(USE_LIBZSTD "Use libzstd to enable CLOD_ZSTD support" ON)
if (USE_LIBZSTD)
    find_package(zstd REQUIRED)
    target_link_libraries(clod PRIVATE zstd::libzstd_shared)
    set(HAVE_LIBZSTD 1)
endif ()

option(USE_LIBBZ2 "Use libbz2 to enable CLOD_BZIP2 support" ON)
if (USE_LIBBZ2)
    find_package(BZip2 REQUIRED)
    target_link_libraries(clod PRIVATE BZip2::BZip2)
    set(HAVE_LIBBZ2 1)
endif ()

configure_file(compression_config.h.in compression_config.h)
target_include_directories(clod PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")


libclod_test(uniform_behavior)
