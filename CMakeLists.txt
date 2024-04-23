cmake_minimum_required(VERSION 3.14)

project(vSID LANGUAGES C CXX VERSION "0.8.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_AFXDLL)
set(CMAKE_MFC_FLAG 1)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

set(SRCS
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

add_library(vSID WIN32 ${SRCS})
target_link_libraries(vSID ${CMAKE_SOURCE_DIR}/include/es/EuroScopePlugInDLL.lib)

install(FILES ${CMAKE_SOURCE_DIR}/vSidConfig.json DESTINATION ${CMAKE_BINARY_DIR}/vSidConfig.json)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR}/config)