cmake_minimum_required(VERSION 3.14)
project(Prism_UI VERSION 1.0.0 LANGUAGES CXX)

set(PRISM_UI_LIBRARY_NAME Prism_UI)

set(UI_SOURCES
    main_dock_ui.cpp
    scene_hierarchy_ui.cpp
    menu_bar_ui.cpp
    camera_settings_ui.cpp
)

set(UI_HEADERS
    public/ui/main_dock_ui.hpp
    public/ui/scene_hierarchy_ui.hpp
    public/ui/menu_bar_ui.hpp
    public/ui/camera_settings_ui.hpp
)

add_library(${PRISM_UI_LIBRARY_NAME} STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
)

add_dependencies(
    ${PRISM_UI_LIBRARY_NAME}
        imgui
        EnTT
        Prism_Resources
        Prism_Loaders
        Prism_Components
)

target_link_libraries(${PRISM_UI_LIBRARY_NAME}
    PUBLIC
        imgui
        EnTT
        Prism_Resources
        Prism_Loaders
        Prism_Components
)

target_include_directories(${PRISM_UI_LIBRARY_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/public
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)