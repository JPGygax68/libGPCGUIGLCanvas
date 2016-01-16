cmake_minimum_required(VERSION 3.0)

# Project name
project(libGPCGUIGLRenderer_Proj)

# Version (semantic: major, minor, patch-level)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

add_subdirectory(thirdparty/GLUtils)
#add_subdirectory(thirdparty/libGPCGUICanvas/lib) # TODO: correct to use subdirectory instead of package directory ?
#add_subdirectory(thirdparty/libGPCGUICanvas/testimage)
add_subdirectory(thirdparty/libGPCFonts)
add_subdirectory(thirdparty/libGPCGLWrappers/lib)
add_subdirectory(thirdparty/libGPCGUIRenderer/lib)

add_subdirectory(lib)

option(Build_TestApp "Build test application" OFF)

if (Build_TestApp)

	add_subdirectory(thirdparty/glbinding)
	
	add_subdirectory(thirdparty/libGPCGUIRenderer/testimage)
	
	# TODO: the following produces a (non-fatal) error complaining about the unavailability
	# of a header file called "vertex.glsl.h", which is part of the dependency target 
	# libGPCGUIGLCanvas but is a generated file.
	add_subdirectory(testapp)

endif()

#------------------------------------------------
# INSTALLATION
#

if (0)
install(TARGETS libGPCGUIGLRenderer
	EXPORT libGPCGUIGLRenderer
	ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
)

# Install the ...config.cmake and ...config-version.cmake files
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libGPCGUIGLRenderer-config.cmake"
  "${PROJECT_BINARY_DIR}/libGPCGUIGLRenderer-config-version.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}"
)
 
 # Install the export set for use with the install-tree
install(EXPORT libGPCGUIGLRenderer
	DESTINATION "${INSTALL_CMAKE_DIR}" 
	EXPORT_LINK_INTERFACE_LIBRARIES
)
endif()

# TODO: install testapp