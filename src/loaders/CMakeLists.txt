cmake_minimum_required(VERSION 3.14)
project(Prism_Loaders VERSION 1.0.0 LANGUAGES CXX)

set(PRISM_LOADERS_LIBRARY_NAME Prism_Loaders)

set(LOADERS_SOURCES
    imgui_loader.cpp
    mesh_loader.cpp
    window_loader.cpp
    vulkan_loader.cpp
    cpu_raytrace_result_loader.cpp
    cpu_bvh_build_morton_loader.cpp
    cpu_bvh_build_sah_loader.cpp
)

set(LOADERS_HEADERS
    public/loaders/imgui_loader.hpp
    public/loaders/mesh_loader.hpp
    public/loaders/window_loader.hpp
    public/loaders/vulkan_loader.hpp
    public/loaders/cpu_raytrace_result_loader.hpp
    public/loaders/cpu_bvh_build_morton_loader.hpp
    public/loaders/cpu_bvh_build_sah_loader.hpp
)

add_library(${PRISM_LOADERS_LIBRARY_NAME} STATIC
    ${LOADERS_SOURCES}
    ${LOADERS_HEADERS}
)

add_dependencies(
    ${PRISM_LOADERS_LIBRARY_NAME}
        glfw
        imgui
        assimp
        Prism_Resources
        Prism_Utils
        volk
        VulkanMemoryAllocator
        stb
)

target_link_libraries(${PRISM_LOADERS_LIBRARY_NAME}
    PUBLIC
        glfw
        imgui
        assimp
        Prism_Resources
        Prism_Utils
        volk
        VulkanMemoryAllocator
        stb
)

target_include_directories(${PRISM_LOADERS_LIBRARY_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/public
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)