# Dependencies CMakeLists.txt
add_subdirectory(entt)
add_subdirectory(glfw)
add_subdirectory(imgui)
add_subdirectory(glm)
add_subdirectory(assimp)
add_subdirectory(ImGuizmo)



# Create GLAD library
add_library(glad STATIC "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c")

target_include_directories(glad
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan libraries:   ${Vulkan_LIBRARIES}")

# Mac-specific configurations
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    find_library(METAL_LIBRARY Metal)

    set(MAC_LIBRARIES
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${METAL_LIBRARY}
    )
endif()

message(STATUS "GLAD library configured successfully")


set(PRISM_DEPENDENCIES 
    glad
    glfw
    glm
    imgui
    ImGuizmo
    ${OPENGL_LIBRARIES}
    ${MAC_LIBRARIES}
    ${Vulkan_LIBRARIES}
    PARENT_SCOPE
)

set(PRISM_DEPENDENCIES_INCLUDE_DIRS
    ${Vulkan_INCLUDE_DIRS}
    PARENT_SCOPE
)