cmake_minimum_required(VERSION 3.22)
project(Prism_Shaders)



# Find glslangValidator
find_program(GLSLANG_VALIDATOR glslangValidator)
if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found! Please install Vulkan SDK.")
endif()

# Directory to store compiled shaders
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

if(WIN32)
    set(NULL_DEVICE NUL)
else()
    set(NULL_DEVICE /dev/null)
endif()

add_library(Prism_Shaders INTERFACE)

# Macro to create a shader target
macro(create_shader FILE)
    # Extract filename and extension
    get_filename_component(NAME ${FILE} NAME_WE)
    get_filename_component(EXT ${FILE} EXT)
    string(TOUPPER ${EXT} EXT_UPPER)
    string(REPLACE "." "" EXT_UPPER ${EXT_UPPER})

    # Output SPIR-V path
    set(OUTPUT_SPV "${SHADER_OUTPUT_DIR}/${NAME}${EXT}.spv")

    # Relative path for nicer compile message
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${FILE})

    # Compile shader to SPIR-V
    add_custom_command(
        OUTPUT ${OUTPUT_SPV}
        COMMAND ${GLSLANG_VALIDATOR} -V ${FILE} -o ${OUTPUT_SPV} 1> "${NULL_DEVICE}" # -q make it quietly, skipping stdout
        DEPENDS ${FILE}
        BYPRODUCTS ${OUTPUT_SPV}    
        COMMENT "Compiling shader ${REL_PATH} -> ${OUTPUT_SPV}"
        VERBATIM
    )

    # Collect all shaders
    set_property(GLOBAL APPEND PROPERTY ALL_SHADERS ${OUTPUT_SPV})
    
    string(TOUPPER ${NAME} NAME_UPPER)
    # Expose shader path to C++ code
    message(STATUS "Adding compile definition: ${NAME_UPPER}_${EXT_UPPER}_SHADER_PATH=\"${OUTPUT_SPV}\"")
    target_compile_definitions(Prism_Shaders INTERFACE
        "${NAME_UPPER}_${EXT_UPPER}_SHADER_PATH=\"${OUTPUT_SPV}\""
    )
endmacro()

# Automatically find all shaders in the source folder
file(GLOB_RECURSE SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.geom"
)

# Create shader targets
foreach(SHADER ${SHADER_FILES})
    create_shader(${SHADER})
endforeach()

# Single target to build all shaders
get_property(ALL_SHADERS GLOBAL PROPERTY ALL_SHADERS)
add_custom_target(Prism_Shaders_Compile ALL DEPENDS ${ALL_SHADERS})
    
add_dependencies(Prism_Shaders Prism_Shaders_Compile)

target_sources(Prism_Shaders PRIVATE ${SHADER_FILES})