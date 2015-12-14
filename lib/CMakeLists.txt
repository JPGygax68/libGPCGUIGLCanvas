project(libGPCGUIGLRenderer)

set(${PROJECT_NAME}_VERSION 0.1.0)

set(SHADER_FILES "src/vertex.glsl" "src/fragment.glsl")

add_library(${PROJECT_NAME} STATIC 
  "src/renderer.cpp"
  "include/gpc/gui/gl/renderer.hpp"
  ${SHADER_FILES}
)

target_include_directories(${PROJECT_NAME} BEFORE
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#--------------------------------------
# Embed shader files
#

source_group("Shader files" FILES ${SHADER_FILES})

# TODO: make this into a CMake module

set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(${PROJECT_NAME} PUBLIC ${GENERATED})

# Find the GPC Bin2C utility
find_package(GPCBin2C REQUIRED)

# Add a custom target and a dependency for each shader file

foreach(shader ${SHADER_FILES})
  get_filename_component(name "${shader}" NAME)
  set(shader_header "${GENERATED}/${name}.h")
  add_custom_command(
    OUTPUT ${shader_header}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
    COMMAND GPCBin2C --input=${CMAKE_CURRENT_SOURCE_DIR}/${shader} --output=${shader_header}
  )
  target_sources(${PROJECT_NAME} PUBLIC ${shader_header})
endforeach()

#--------------------------------------
# Libraries
#

target_link_libraries(${PROJECT_NAME} PUBLIC libGPCGUIRenderer)

if (NOT TARGET libGPCGLWrappers)
	find_package(libGPCGLWrappers REQUIRED)
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC libGPCGLWrappers)

#find_package(libGPCGLUtils REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC libGPCGLUtils)

# All our calls to OpenGL are from within inlined code (header files),
# so we don't link in any OpenGL bindings
if (0)

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENGL_LIBRARIES})

find_package(GLEW REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ${GLEW_LIBRARIES})

endif()

# GPC Fonts (we need the Rasterized Font module)

find_package(libGPCFonts REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC libGPCFonts)

# Cereal

# Cereal does not have a package and must be made available by other means


#------------------------------------------------
# Config files

# Create the config file

# ... for the build tree
set(CONF_CMAKE_DIR "${PROJECT_BINARY_DIR}")
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file(${PROJECT_NAME}-config.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake" @ONLY)

# ... for the install tree
message("INSTALL_CMAKE_DIR: ${INSTALL_CMAKE_DIR}")
message("INSTALL_INCLUDE_DIR: ${INSTALL_INCLUDE_DIR}")
file(RELATIVE_PATH rel_incdir "${INSTALL_CMAKE_DIR}" "${INSTALL_INCLUDE_DIR}")
set(CONF_INCLUDE_DIRS "\${${PROJECT_NAME}_CMAKE_DIR}/${rel_incdir}")
configure_file(${PROJECT_NAME}-config.cmake.in "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}-config.cmake" @ONLY)

# ... for both
set(PACKAGE_VERSION ${})
configure_file(${PROJECT_NAME}-config-version.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake" @ONLY)

