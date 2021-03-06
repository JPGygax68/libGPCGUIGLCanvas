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

set(GENERATED "${CMAKE_BINARY_DIR}/generated")
target_include_directories(${PROJECT_NAME} PUBLIC ${GENERATED})

# Find the GPC Bin2C utility
if (NOT TARGET gpcbin2c)
    find_package(gpcbin2c REQUIRED)
endif()

# Add a custom target and a dependency for each shader file

foreach(shader ${SHADER_FILES})
  get_filename_component(name "${shader}" NAME)
  set(shader_header "${GENERATED}/gpc/gui/gl/${name}.h")
  add_custom_command(
    OUTPUT ${shader_header}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
    COMMAND gpcbin2c --input=${CMAKE_CURRENT_SOURCE_DIR}/${shader} --output=${shader_header}
  )
  target_sources(${PROJECT_NAME} PUBLIC ${shader_header})
endforeach()

#--------------------------------------
# Dependencies
#

if (NOT TARGET libGPCGUIRenderer)
	message(FATAL_ERROR "libGPCGUIRenderer not defined as a target")
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC libGPCGUIRenderer)

if (NOT TARGET libGPCGLWrappers)
	message(FATAL_ERROR "libGPCGLWrappers not defined as a target")
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC libGPCGLWrappers)

if (NOT TARGET libGPCGLUtils)
    message(FATAL_ERROR "libGPCGLUtils not defined as a target")
endif()
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

if (NOT TARGET libGPCFonts)
  find_package(libGPCFonts CONFIG REQUIRED)
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC libGPCFonts)

# Cereal

# Cereal does not have a package and must be made available by other means
