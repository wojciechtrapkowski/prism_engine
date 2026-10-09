cmake_minimum_required(VERSION 3.14)
project(Prism_Assets VERSION 1.0.0 LANGUAGES CXX)

set(PRISM_ASSETS_LIBRARY_NAME Prism_Assets)

set(ASSETS_HEADERS
    public/assets/box.hpp
)


add_library(${PRISM_ASSETS_LIBRARY_NAME} INTERFACE
    ${ASSETS_HEADERS}
)

# add_dependencies(
#     ${PRISM_ASSETS_LIBRARY_NAME}
       
# )

# target_link_libraries(${PRISM_ASSETS_LIBRARY_NAME}
#     INTERFACE
        
# )

target_include_directories(${PRISM_ASSETS_LIBRARY_NAME}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/public
)