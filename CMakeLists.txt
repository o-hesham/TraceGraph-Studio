cmake_minimum_required(VERSION 3.22)

project(TraceGraphStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets Concurrent)
qt_standard_project_setup(REQUIRES 6.11)

add_subdirectory(src)
