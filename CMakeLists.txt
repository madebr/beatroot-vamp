cmake_minimum_required(VERSION 3.15)
project(beatroot_vamp CXX)

include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
cmake_dependent_option(BUILD_VAMP_PLUGIN "Build vamp plugin" ON "NOT BUILD_SHARED_LIBS" OFF)

if(BUILD_SHARED_LIBS)
    set(beatroot_export_name "beatroot")
    set(beatroot_shared_static "shared")
else()
    set(beatroot_export_name "beatroot-static")
    set(beatroot_shared_static "static")
endif()

set(BEATROOT_HEADERS
    Agent.h
    AgentList.h
    BeatRootProcessor.h
    BeatTracker.h
    Induction.h
    Peaks.h
)
add_library(beatroot
    Agent.cpp
    AgentList.cpp
    BeatRootProcessor.cpp
    BeatTracker.cpp
    Event.h
    Induction.cpp
    Peaks.cpp
    ${BEATROOT_HEADERS}
)
add_library(beatroot::${beatroot_export_name} ALIAS beatroot)
target_include_directories(beatroot PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/beatroot>"
)
set_target_properties(beatroot
    PROPERTIES
        EXPORT_NAME ${beatroot_export_name}
)
if(MSVC AND NOT BUILD_SHARED_LIBS)
    set_target_properties(beatroot
        PROPERTIES
            OUTPUT_NAME beatroot-static
    )
endif()

set(BEATROOT_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/beatroot")

configure_package_config_file(beatroot-config.cmake.in beatroot-config.cmake
    INSTALL_DESTINATION "${BEATROOT_INSTALL_CMAKEDIR}"
)

install(TARGETS beatroot EXPORT beatrootExports
    ARCHIVE DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(FILES ${BEATROOT_HEADERS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/beatroot"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/beatroot-config.cmake"
    DESTINATION "${BEATROOT_INSTALL_CMAKEDIR}"
)
install(EXPORT beatrootExports
    NAMESPACE "beatroot::"
    DESTINATION "${BEATROOT_INSTALL_CMAKEDIR}"
    FILE "beatroot-${beatroot_shared_static}-targets.cmake"
)

if(BUILD_SHARED_LIBS)
endif()

if(BUILD_VAMP_PLUGIN)
    find_package(vamp-sdk REQUIRED)

    set(BEATROOT_VAMP_HEADERS
        BeatRootVampPlugin.h
    )
    add_library(beatroot-vamp SHARED
        BeatRootVampPlugin.cpp
        ${BEATROOT_VAMP_HEADERS}
    )
    target_link_libraries(beatroot-vamp PRIVATE vamp-sdk::vamp-sdk)
    if(APPLE)
        target_link_options(beatroot-vamp
            PRIVATE
                -exported_symbols_list "${CMAKE_CURRENT_SOURCE_DIR}/vamp-plugin.list"
        )
    endif()
    if(MINGW)
        target_link_options(beatroot-vamp
            PRIVATE
                -Wl,--static-libgcc
                "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/vamp-plugin.map"
                -fno-exceptions
        )
    endif()

    install(TARGETS beatroot-vamp
        ARCHIVE DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    install(FILES ${BEATROOT_VAMP_HEADERS}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/beatroot"
    )
endif()
