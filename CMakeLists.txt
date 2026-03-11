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

enable_testing()

add_executable(vg_architect_tests
    tests/test_main.cpp
    tests/test_geometry_export.cpp
    tests/test_wall_graph.cpp
    tests/test_half_edge_topology.cpp
    tests/test_face_extraction.cpp
)

target_include_directories(vg_architect_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external
)

if(MSVC)
    target_compile_options(vg_architect_tests PRIVATE /W4 /permissive-)
endif()

add_test(NAME AllTests COMMAND vg_architect_tests)
