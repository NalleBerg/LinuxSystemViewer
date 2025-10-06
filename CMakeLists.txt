cmake_minimum_required(VERSION 3.16)
project(LSV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Find Qt6 installation on Linux
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network)

# Set up Qt6 automoc, autouic, autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Create executable (using lsv.cpp file and embedded resources)
add_executable(LSV lsv.cpp memory.cpp resources.qrc)

# Link Qt libraries
target_link_libraries(LSV PRIVATE Qt6::Widgets Qt6::Core Qt6::Network)
