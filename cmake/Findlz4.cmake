include(FindPackageHandleStandardArgs)
find_package(PkgConfig)

if (PkgConfig_FOUND)
    pkg_check_modules(PC_LZ4 QUIET liblz4)
endif ()

find_path(LZ4_INCLUDE_DIR
    NAMES lz4frame.h
    HINTS ${PC_LZ4_INCLUDE_DIRS}
)

find_library(LZ4_LIBRARY
    NAMES lz4
    HINTS ${PC_LZ4_LIBRARY_DIRS}
)

find_package_handle_standard_args(lz4
    REQUIRED_VARS
        LZ4_INCLUDE_DIR
        LZ4_LIBRARY
)

if (lz4_FOUND AND NOT TARGET LZ4::lz4)
    add_library(LZ4::lz4 UNKNOWN IMPORTED)
    set_target_properties(LZ4::lz4 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
        IMPORTED_LOCATION "${LZ4_LIBRARY}"
    )
endif ()
