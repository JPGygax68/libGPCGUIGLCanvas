
set(SHADER_FILES "src/vertex.glsl" "src/fragment.glsl")

add_library(libGPCGUIGLCanvas STATIC 
  "src/canvas.cpp"
  "include/gpc/gui/gl/canvas.hpp"
  ${SHADER_FILES}
)

target_include_directories(libGPCGUIGLCanvas BEFORE
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
target_include_directories(libGPCGUIGLCanvas PUBLIC ${GENERATED})

# Find the GPC Bin2C utility
find_package(GPCBin2C REQUIRED)

# Add a custom target and a dependency for each shader file

foreach(shader ${SHADER_FILES})
  get_filename_component(name "${shader}" NAME)
  set(shader_file "${GENERATED}/${name}.h")
  add_custom_command(
    OUTPUT ${shader_file}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
    COMMAND GPCBin2C --input=${CMAKE_CURRENT_SOURCE_DIR}/${shader} --output=${shader_file}
  )
  target_sources(libGPCGUIGLCanvas PUBLIC ${shader_file})
endforeach()

#--------------------------------------
# Libraries
#

find_package(libGPCGLWrappers REQUIRED)
target_link_libraries(libGPCGUIGLCanvas PRIVATE libGPCGLWrappers)

#find_package(libGPCGLUtils REQUIRED)
target_link_libraries(libGPCGUIGLCanvas PUBLIC libGPCGLUtils)

find_package(GLEW REQUIRED)
target_link_libraries(libGPCGUIGLCanvas PRIVATE ${GLEW_LIBRARIES})

# GPC Fonts (we need the Rasterized Font module)

find_package(libGPCFonts REQUIRED)
target_link_libraries(libGPCGUIGLCanvas PUBLIC libGPCFonts)

# Cereal

# Cereal does not have a package and must be made available by other means

