cmake_minimum_required(VERSION 3.20)

project(AirGraffiti)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL3_DIR "${CMAKE_SOURCE_DIR}/external/SDL3-3.4.14")

add_executable(AirGraffiti
    src/main.cpp
    src/application.cpp
    src/brush.cpp
    src/canvas.cpp
    src/inputmanager.cpp
    src/pointer.cpp
    src/renderer.cpp
)

target_include_directories(AirGraffiti PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${SDL3_DIR}/include"
)

find_library(SDL3_LIBRARY
    NAMES SDL3
    PATHS
        "${SDL3_DIR}/lib"
        "${SDL3_DIR}/lib/x64"
    NO_DEFAULT_PATH
)

if(NOT SDL3_LIBRARY)
    message(FATAL_ERROR "Could not find SDL3.lib")
endif()

target_link_libraries(AirGraffiti PRIVATE
    "${SDL3_LIBRARY}"
)

find_file(SDL3_DLL
    NAMES SDL3.dll
    PATHS
        "${SDL3_DIR}/lib"
        "${SDL3_DIR}/lib/x64"
    NO_DEFAULT_PATH
)

if(SDL3_DLL)
    add_custom_command(TARGET AirGraffiti POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_DLL}"
        "$<TARGET_FILE_DIR:AirGraffiti>"
    )
endif()