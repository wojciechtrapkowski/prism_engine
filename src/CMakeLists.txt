add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:RELEASE>
)

add_executable(PrismMain main.cpp)

# Add subdirectories
add_subdirectory(context)
add_subdirectory(loaders)
# add_subdirectory(controllers)
add_subdirectory(shaders)
add_subdirectory(resources)
add_subdirectory(systems)
add_subdirectory(models)
add_subdirectory(events)
add_subdirectory(components)
add_subdirectory(utils)
add_subdirectory(managers)
add_subdirectory(ui)

# Link required libraries
target_link_libraries(PrismMain
    PRIVATE
        ${Vulkan}
        Prism_Context
)

message(STATUS "PRISM_DEPENDENCIES_INCLUDE_DIRS = ${PRISM_DEPENDENCIES_INCLUDE_DIRS}")
message(STATUS "PRISM_DEPENDENCIES = ${PRISM_DEPENDENCIES}")

# Mac-specific configurations
if(APPLE)
    target_link_libraries(PrismMain PRIVATE ${MAC_LIBRARIES})
endif()

target_include_directories(PrismMain PRIVATE ${PRISM_DEPENDENCIES_INCLUDE_DIRS})

# Set the output directory
set_target_properties(PrismMain
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install the executable
install(TARGETS PrismMain
    RUNTIME DESTINATION bin
)

message(STATUS "Temporary build setup configured for main.cpp")