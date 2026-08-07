cmake_minimum_required(VERSION 3.20)

project(AirGraffiti)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------
# SDL3
# --------------------------------------------------

set(SDL3_DIR "${CMAKE_SOURCE_DIR}/external/SDL3-3.4.14")

# --------------------------------------------------
# Dear ImGui
# --------------------------------------------------

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")

# --------------------------------------------------
# AirGraffiti + ImGui source files
# --------------------------------------------------

add_executable(AirGraffiti
    src/main.cpp
    src/application.cpp
    src/brush.cpp
    src/canvas.cpp
    src/inputmanager.cpp
    src/pointer.cpp
    src/renderer.cpp
    src/ui.cpp

    # Dear ImGui core
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp

    # Dear ImGui SDL3 backend
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp

    # Dear ImGui SDL Renderer 3 backend
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# --------------------------------------------------
# Include directories
# --------------------------------------------------

target_include_directories(AirGraffiti PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${SDL3_DIR}/include"

    # Dear ImGui
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

# --------------------------------------------------
# Find SDL3
# --------------------------------------------------

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

# --------------------------------------------------
# Link SDL3
# --------------------------------------------------

target_link_libraries(AirGraffiti PRIVATE
    "${SDL3_LIBRARY}"
)

# --------------------------------------------------
# Copy SDL3.dll next to AirGraffiti.exe
# --------------------------------------------------

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