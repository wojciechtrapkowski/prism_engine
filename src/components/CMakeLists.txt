cmake_minimum_required(VERSION 3.14)
project(Prism_Components VERSION 1.0.0 LANGUAGES CXX)

set(PRISM_COMPONENTS_LIBRARY_NAME Prism_Components)

set(COMPONENTS_HEADERS
    public/components/mesh.hpp
    public/components/tags.hpp
    public/components/transform.hpp
    public/components/camera.hpp
    public/components/camera_control.hpp
    public/components/systems_settings.hpp
    public/components/name.hpp
    public/components/aabb.hpp
)


add_library(${PRISM_COMPONENTS_LIBRARY_NAME} INTERFACE
    ${COMPONENTS_HEADERS}
)

add_dependencies(
    ${PRISM_COMPONENTS_LIBRARY_NAME}
        EnTT
        glm
)

target_link_libraries(${PRISM_COMPONENTS_LIBRARY_NAME}
    INTERFACE
        EnTT
        glm
)

target_include_directories(${PRISM_COMPONENTS_LIBRARY_NAME}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/public
)