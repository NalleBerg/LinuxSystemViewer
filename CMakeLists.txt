cmake_minimum_required(VERSION 3.16)
project(LSV VERSION 0.6.0)

set(CMAKE_CXX_STANDARD 17)

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
    pc_tab.cpp  
    memory_tab.cpp
)

# Link Qt libraries using keyword signature to match Qt6's internal usage
target_link_libraries(LSV PRIVATE
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network
)

# Resources (if lsv.png exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsv.png")
    qt6_add_resources(LSV "resources"
        PREFIX "/"
        FILES
            lsv.png
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