cmake_minimum_required(VERSION 3.16)
project(LSV VERSION 0.7.0)

set(CMAKE_CXX_STANDARD 17)

# Option to compile debug logger into the binary. Default OFF for releases so
# logging code can be compiled out of production builds.
option(LSV_ENABLE_DEBUG_LOGGER "Compile-in the optional debug logger (writes to temp when enabled)" OFF)
if(LSV_ENABLE_DEBUG_LOGGER)
    message(STATUS "LSV: Debug logger ENABLED in this build")
    add_compile_definitions(LSV_ENABLE_DEBUG_LOGGER)
else()
    message(STATUS "LSV: Debug logger DISABLED (default)")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt6_standard_project_setup()

# Main executable with all source files
qt6_add_executable(LSV
    lsv.cpp
    multitabs.cpp
    ctrlw.cpp
    tab_widget_base.cpp
    summary_tab.cpp
    generic_tab.cpp
    os_tab.cpp
    cpu_tab.cpp
    audio_tab.cpp
    graphics_tab.cpp
    ports_tab.cpp
    screen_tab.cpp
    peripherals_tab.cpp
    windowing_tab.cpp
    motherboard_tab.cpp
    storage_tab.cpp
    about_tab.cpp
    network_tab.cpp
    network_geek.cpp
    pc_tab.cpp  
    memory_tab.cpp
)

# Link Qt libraries using keyword signature to match Qt6's internal usage
target_link_libraries(LSV PRIVATE
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network
)

# Resources: always include the embedded license and GNU icon so packaged
# artifacts contain the GPLv2 text and icon even if local images aren't
# present in the source tree. Add optional local images only when they
# exist to avoid rcc failures when the developer hasn't provided them.

# Core resources (always present)
qt6_add_resources(LSV "resources_core"
    PREFIX "/"
    FILES
        gpl2.txt
        gnu_icon.png
)

# Optional local images: register them only if they exist so rcc doesn't
# fail during builds on systems that don't have the workspace images.
set(LSV_IMAGE_FILES)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png")
    list(APPEND LSV_IMAGE_FILES lsv.png)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsv.svg")
    list(APPEND LSV_IMAGE_FILES lsv.svg)
endif()
if(LSV_IMAGE_FILES)
    qt6_add_resources(LSV "resources_images"
        PREFIX "/"
        FILES
            ${LSV_IMAGE_FILES}
    )
endif()

# Optionally include compiled translations if present in i18n/
set(LSV_I18N_FILES)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/i18n")
    file(GLOB QM_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/i18n "${CMAKE_CURRENT_SOURCE_DIR}/i18n/*.qm")
    if(QM_FILES)
        foreach(qm ${QM_FILES})
            list(APPEND LSV_I18N_FILES "i18n/${qm}")
        endforeach()
    endif()
endif()
if(LSV_I18N_FILES)
    qt6_add_resources(LSV "resources_i18n"
        PREFIX "/"
        FILES
            ${LSV_I18N_FILES}
    )
endif()

# Enable debug symbols
set_target_properties(LSV PROPERTIES
    CMAKE_BUILD_TYPE Debug
)

# Small privileged helper (optional): builds a tiny launcher which can be
# installed setuid-root by the system administrator to reliably exec the
# main LSV binary as root while preserving the user's environment vars.
add_subdirectory(priv)

# Install targets and desktop integration for packaging
install(TARGETS LSV RUNTIME DESTINATION bin)

# Install desktop file and icons so packages integrate with the desktop
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsv.desktop")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.desktop" DESTINATION share/applications)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsv-512.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv-512.png" DESTINATION share/icons/hicolor/512x512/apps RENAME lsv.png)
endif()

# CPack packaging configuration: produce .deb and .rpm packages
set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_PACKAGE_NAME "lsv")
set(CPACK_PACKAGE_VENDOR "Linux System Viewer")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Linux System Viewer - hardware and system info GUI")
set(CPACK_PACKAGE_DESCRIPTION "Linux System Viewer (LSV) is a Qt-based utility that displays hardware and system information in a tabbed GUI.")
set(CPACK_PACKAGE_CONTACT "prog@nalle.no")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Make packaged file names predictable: lsv-<version>-<arch>.<ext>
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}")
set(CPACK_RPM_PACKAGE_RELEASE "1")

# Let CPack attempt to detect library dependencies for Debian packages
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# Default install prefix for packages
set(CPACK_PACKAGING_INSTALL_PREFIX "/")

include(CPack)