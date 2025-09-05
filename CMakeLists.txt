cmake_minimum_required(VERSION 3.23)

project(Prism VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CSS_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PRISM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(dependencies)
add_subdirectory(src)

message(STATUS "Prism Graphics Engine configured successfully!")

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT PrismMain)