cmake_minimum_required(VERSION 3.20)

project(AirGraffiti)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL3_DIR
    "${CMAKE_SOURCE_DIR}/external/SDL3-3.4.14"
)

set(IMGUI_DIR
    "${CMAKE_SOURCE_DIR}/external/imgui"
)

set(BUILD_EXAMPLE OFF CACHE BOOL "Build Wiiuse example application")
set(BUILD_EXAMPLE_SDL OFF CACHE BOOL "Build Wiiuse SDL example application")
set(INSTALL_EXAMPLES OFF CACHE BOOL "Install Wiiuse example applications")

add_subdirectory(
    "${CMAKE_SOURCE_DIR}/external/wiiuse"
    "${CMAKE_BINARY_DIR}/wiiuse"
)

add_executable(AirGraffiti

    src/main.cpp
    src/application.cpp
    src/brush.cpp
    src/canvas.cpp
    src/inputmanager.cpp
    src/pointer.cpp
    src/renderer.cpp
    src/ui.cpp
    src/wiimotemanager.cpp
    src/wiimotecalibration.cpp
    src/irfusion.cpp

    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp

    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(AirGraffiti PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${SDL3_DIR}/include"
    "${CMAKE_SOURCE_DIR}/external/wiiuse/src"
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

target_compile_definitions(AirGraffiti PRIVATE
    WIIUSE_STATIC
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
    wiiuse
)

if(WIN32)
    target_link_libraries(AirGraffiti PRIVATE
        hid
        setupapi
    )
endif()

find_file(SDL3_DLL
    NAMES SDL3.dll
    PATHS
        "${SDL3_DIR}/lib"
        "${SDL3_DIR}/lib/x64"
    NO_DEFAULT_PATH
)

if(SDL3_DLL)
    add_custom_command(
        TARGET AirGraffiti
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_DLL}"
            "$<TARGET_FILE_DIR:AirGraffiti>"
    )
endif()

message(STATUS "========================================")
message(STATUS "AirGraffiti configuration")
message(STATUS "========================================")
message(STATUS "SDL3:")
message(STATUS "  Library: ${SDL3_LIBRARY}")
message(STATUS "")
message(STATUS "Wiiuse:")
message(STATUS "  Source: ${CMAKE_SOURCE_DIR}/external/wiiuse")
message(STATUS "  Static library: enabled")
message(STATUS "  IR support: enabled")
message(STATUS "  Maximum remotes: 5")
message(STATUS "  Default remotes: 2")
message(STATUS "")
message(STATUS "Dear ImGui:")
message(STATUS "  Source: ${IMGUI_DIR}")
message(STATUS "  Enabled")
message(STATUS "========================================")