cmake_minimum_required(VERSION 3.10)

project(vSID LANGUAGES C CXX VERSION "0.8.0")
set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_AFXDLL)

# Add the directory containing example.lib to the list of directories CMake searches for libraries
link_directories(${CMAKE_SOURCE_DIR}/include/es)

# Add the header files directory if necessary
include_directories(${CMAKE_SOURCE_DIR}/include)

# Define your DLL's source files
set(SOURCES 
    area.cpp
    configparser.cpp
    flightplan.cpp
    messageHandler.cpp
    pch.cpp
    sid.cpp
    timeHandler.cpp
    utils.cpp
    vSID.cpp
    vSIDPlugin.cpp
)

# Define your DLL's target
add_library(vSID SHARED ${SOURCES})

# Link your DLL with example.lib
#target_link_libraries(vSID ${CMAKE_SOURCE_DIR}/include/es/EuroScopePlugInDLL.lib)
target_link_libraries(vSID EuroScopePlugInDLL.lib)
