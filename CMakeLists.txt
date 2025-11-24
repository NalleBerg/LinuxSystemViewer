cmake_minimum_required(VERSION 3.16)
project(LSV VERSION 0.17.0)

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

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia)
find_package(ALSA REQUIRED)
include_directories(${ALSA_INCLUDE_DIRS})

qt6_standard_project_setup()

# Main executable with all source files
qt6_add_executable(LSV
    lsv.cpp
    version.cpp
    multitabs.cpp
    ctrlw.cpp
    tab_widget_base.cpp
    summary_tab.cpp
    generic_tab.cpp
    os_tab.cpp
    cpu_tab.cpp
    audio_tab.cpp
    alsa_player.cpp
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
    Qt6::Multimedia
    ${ALSA_LIBRARIES}
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
        # Exclude work-in-progress translations from the release bundle.
        # Icelandic is placed in i18n_pending for now and must not be
        # embedded into the release resources.
        list(REMOVE_ITEM QM_FILES "lsv_is.qm")
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
# Only include the helper if the source directory exists. This avoids a
# hard failure for developer checkouts that don't include the helper.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/priv")
    add_subdirectory(priv)
else()
    message(STATUS "Optional 'priv' helper not found; skipping privileged helper build")
endif()

# Install targets and desktop integration for packaging
install(TARGETS LSV RUNTIME DESTINATION bin)

# Install desktop file and icons so packages integrate with the desktop
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsv.desktop")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.desktop" DESTINATION share/applications)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsv-512.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv-512.png" DESTINATION share/icons/hicolor/512x512/apps RENAME lsv.png)
endif()

# Also install the generic `lsv.png` into common hicolor sizes so the
# produced .deb/.rpm packages include the application icon for the menu
# on target systems even when distro packagers don't resize images.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/16x16/apps RENAME lsv.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/22x22/apps RENAME lsv.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/24x24/apps RENAME lsv.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/32x32/apps RENAME lsv.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/48x48/apps RENAME lsv.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/64x64/apps RENAME lsv.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/128x128/apps RENAME lsv.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/256x256/apps RENAME lsv.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png" DESTINATION share/icons/hicolor/512x512/apps RENAME lsv.png)
    
    # Install sound files for audio testing
    install(FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/sounds/a440.wav"
        "${CMAKE_CURRENT_SOURCE_DIR}/sounds/lowc.wav"
        "${CMAKE_CURRENT_SOURCE_DIR}/sounds/left.wav"
        "${CMAKE_CURRENT_SOURCE_DIR}/sounds/right.wav"
        DESTINATION share/lsv/sounds)
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

# Include package maintainer scripts (postinst/postrm) for DEB and RPM
# If the scripts exist in cmake/packaging, instruct CPack to embed them so the
# package manager will run them on install/remove. Paths are absolute so CPack
# can find them during package generation.
set(LSV_PACKAGING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/packaging")
if(EXISTS "${LSV_PACKAGING_DIR}/postinst")
    # Ensure scripts are executable so the package manager can run them
    file(CHMOD "${LSV_PACKAGING_DIR}/postinst" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    if(EXISTS "${LSV_PACKAGING_DIR}/postrm")
        file(CHMOD "${LSV_PACKAGING_DIR}/postrm" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()

    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${LSV_PACKAGING_DIR}/postinst;${LSV_PACKAGING_DIR}/postrm")
    set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${LSV_PACKAGING_DIR}/postinst")
    set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${LSV_PACKAGING_DIR}/postrm")
endif()

# Default install prefix for packages
# Use /usr so installed files end up under /usr/bin and /usr/share (typical
# packaging convention). Previously '/' produced /bin and /share which are not
# where desktop environments expect application icons and desktop files.
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

include(CPack)