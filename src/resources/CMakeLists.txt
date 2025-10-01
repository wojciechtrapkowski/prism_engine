cmake_minimum_required(VERSION 3.14)
project(Prism_Resources VERSION 1.0.0 LANGUAGES CXX)

set(PRISM_RESOURCES_LIBRARY_NAME Prism_Resources)

set(RESOURCES_SOURCES
    imgui_resource.cpp
    mesh_resource.cpp
    scene.cpp
    window_resource.cpp
    vulkan_resource.cpp
    context_resources.cpp
    render_target_resource.cpp
    
    vulkan/vk_command_pool_resource.cpp
    vulkan/vk_framebuffer_resource.cpp
    vulkan/vk_staging_buffer_resource.cpp
)

set(RESOURCES_HEADERS
    public/resources/resource.hpp
    public/resources/imgui_resource.hpp
    public/resources/mesh_resource.hpp
    public/resources/scene.hpp
    public/resources/context_resources.hpp
    public/resources/common_resource.hpp
    public/resources/window_resource.hpp
    public/resources/vulkan_resource.hpp
    public/resources/resource_storage.hpp
    public/resources/render_target_resource.hpp

    public/resources/vulkan/vk_command_pool_resource.hpp
    public/resources/vulkan/vk_framebuffer_resource.hpp
    public/resources/vulkan/vk_buffer_resource.hpp
    public/resources/vulkan/vk_staging_buffer_resource.hpp
)

add_library(${PRISM_RESOURCES_LIBRARY_NAME} STATIC
    ${RESOURCES_SOURCES}
    ${RESOURCES_HEADERS}
)

add_dependencies(
    ${PRISM_RESOURCES_LIBRARY_NAME}
        glfw
        glm
        imgui
        EnTT
        VulkanMemoryAllocator
        Prism_Components
        Prism_Utils
)

target_link_libraries(${PRISM_RESOURCES_LIBRARY_NAME}
    PUBLIC
        glfw
        glm
        imgui
        EnTT
        VulkanMemoryAllocator
        ${Vulkan}
        Prism_Components
        Prism_Utils
)

target_include_directories(${PRISM_RESOURCES_LIBRARY_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/public
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)