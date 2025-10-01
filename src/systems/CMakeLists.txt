cmake_minimum_required(VERSION 3.14)
project(Prism_Systems VERSION 1.0.0 LANGUAGES CXX)

set(PRISM_SYSTEMS_LIBRARY_NAME Prism_Systems)

set(SYSTEMS_SOURCES
    screen_clearing_system.cpp
    mesh_drawing_system.cpp
    present_system.cpp
    input_control_system.cpp
    event_poll_system.cpp
    camera_creation_system.cpp
    fps_motion_control_system.cpp
    common_uniform_update_system.cpp
    ui_drawing_system.cpp
    gizmo_drawing_system.cpp
    window_resize_system.cpp
)

set(SYSTEMS_HEADERS
    public/systems/screen_clearing_system.hpp
    public/systems/mesh_drawing_system.hpp
    public/systems/present_system.hpp
    public/systems/input_control_system.hpp
    public/systems/event_poll_system.hpp
    public/systems/camera_creation_system.hpp
    public/systems/fps_motion_control_system.hpp
    public/systems/common_uniform_update_system.hpp
    public/systems/ui_drawing_system.hpp
    public/systems/gizmo_drawing_system.hpp
    public/systems/window_resize_system.hpp
)

add_library(${PRISM_SYSTEMS_LIBRARY_NAME} STATIC
    ${SYSTEMS_SOURCES}
    ${SYSTEMS_HEADERS}
)

add_dependencies(
    ${PRISM_SYSTEMS_LIBRARY_NAME}
        glfw
        imguizmo
        Prism_Resources
        Prism_Loaders
        Prism_Events
        Prism_Components
        Prism_UI
)

target_link_libraries(${PRISM_SYSTEMS_LIBRARY_NAME}
    PUBLIC
        glfw
        imguizmo
        Prism_Resources
        Prism_Loaders
        Prism_Events
        Prism_Components
        Prism_Utils
        Prism_UI
        Prism_Shaders
        ${Vulkan}
)

target_include_directories(${PRISM_SYSTEMS_LIBRARY_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/public
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)