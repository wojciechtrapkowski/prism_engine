set(IMGUI_DIR ".")

set(IMGUI_SOURCES
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
    )

set(IMGUI_BACKEND_SOURCES
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})

target_include_directories(imgui
    PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${Vulkan_INCLUDE_DIRS}
)

# Link with OpenGL and GLFW
target_link_libraries(imgui
    PUBLIC
        ${OPENGL_LIBRARIES}
        ${Vulkan}
)

# If using GLFW as a target
if(TARGET glfw)
    target_link_libraries(imgui PUBLIC glfw)
else()
    # If GLFW was found as a package
    find_package(glfw3 REQUIRED)
    target_link_libraries(imgui PUBLIC glfw)
endif()

# Set compile definitions for the backend
target_compile_definitions(imgui
    PUBLIC
        IMGUI_IMPL_OPENGL_LOADER_GLAD
)