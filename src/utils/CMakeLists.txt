cmake_minimum_required(VERSION 3.14)
project(Prism_Utils VERSION 1.0.0 LANGUAGES CXX)

set(PRISM_UTILS_LIBRARY_NAME Prism_Utils)

set(UTILS_SOURCES
    vulkan/common.cpp
    vulkan/debug_messenger.cpp
)

set(UTILS_HEADERS
    public/utils/opengl_debug.hpp
    public/utils/vulkan/common.hpp
    public/utils/vulkan/debug_messenger.hpp
)

add_library(${PRISM_UTILS_LIBRARY_NAME} STATIC ${UTILS_SOURCES} ${UTILS_HEADERRS})

add_dependencies(
    ${PRISM_UTILS_LIBRARY_NAME}
    glfw
)

target_link_libraries(
    ${PRISM_UTILS_LIBRARY_NAME} 
    PUBLIC
        glfw
        ${Vulkan}

)

target_include_directories(${PRISM_UTILS_LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/public
)