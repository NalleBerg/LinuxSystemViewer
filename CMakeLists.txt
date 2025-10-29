cmake_minimum_required(VERSION 3.16)
project(LSV VERSION 0.6.5)

set(CMAKE_CXX_STANDARD 17)

# Option to compile debug logger into the binary. Default OFF for releases so
# logging code can be compiled out of production AppImages.
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

# Resources: always include the embedded license and GNU icon so the
# AppImage contains the GPLv2 text and icon even if local images aren't
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

# Enable debug symbols
set_target_properties(LSV PROPERTIES
    CMAKE_BUILD_TYPE Debug
)

# Small privileged helper (optional): builds a tiny launcher which can be
# installed setuid-root by the system administrator to reliably exec the
# main LSV binary as root while preserving the user's environment vars.
add_subdirectory(priv)