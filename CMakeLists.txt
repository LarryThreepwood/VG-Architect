cmake_minimum_required(VERSION 3.24)

project(VGArchitect VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "VG Architect is a Windows-only application.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)

add_executable(vg_architect
    src/main.cpp
)

add_executable(VGArchitect::App ALIAS vg_architect)

target_include_directories(vg_architect
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(vg_architect
    PRIVATE
        vgarchitect::glfw
        vgarchitect::imgui
        glm::glm
        vgarchitect::glad
        OpenGL::GL
)

if(MSVC)
    target_compile_options(vg_architect PRIVATE /W4 /permissive-)
endif()
