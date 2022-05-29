find_path(VAMP_SDK_INCLUDE_DIR
    NAMES vamp-sdk/Plugin.h
)

find_library(VAMP_SDK_LIBRARY
    NAMES vamp-sdk
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(vamp-sdk
    REQUIRED_VARS VAMP_SDK_LIBRARY VAMP_SDK_INCLUDE_DIR
)

if(vamp-sdk_FOUND)
    if(NOT TARGET vamp-sdk::vamp-sdk)
        add_library(vamp-sdk::vamp-sdk UNKNOWN IMPORTED)
        set_target_properties(vamp-sdk::vamp-sdk
            PROPERTIES
                IMPORTED_LOCATION "${VAMP_SDK_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${VAMP_SDK_INCLUDE_DIR}"
        )
    endif()
endif()
