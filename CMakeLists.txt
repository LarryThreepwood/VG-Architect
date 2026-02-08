cmake_minimum_required(VERSION 3.24)

project(VGArchitect VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "VG Architect is a Windows-only application.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(vg_architect
    src/app/main.cpp
)

target_include_directories(vg_architect
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/include
)

if(MSVC)
    target_compile_options(vg_architect PRIVATE /W4 /permissive-)
endif()
