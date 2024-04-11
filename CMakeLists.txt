cmake_minimum_required(VERSION 3.14)

PROJECT(vSID LANGUAGES C CXX VERSION "0.8.0")
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/include
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_AFXDLL")

SET(SOURCE_FILES
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

# FIXME: find with find_package
# find_package(MFC REQUIRED)
link_directories(
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.39.33519\\atlmfc\\lib\\spectre\\x86"
)

ADD_LIBRARY(vSID SHARED ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(vSID ${CMAKE_SOURCE_DIR}/include/es/EuroScopePlugInDLL.lib)