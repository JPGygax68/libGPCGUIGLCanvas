cmake_minimum_required(VERSION 3.0)

# Project name
project(libGPCGUIGLRenderer)

# Version (semantic: major, minor, patch-level)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

add_subdirectory(thirdparty/GLUtils)
add_subdirectory(thirdparty/libGPCGUICanvas/lib) # TODO: correct to use subdirectory instead of package directory ?
add_subdirectory(thirdparty/libGPCGUICanvas/testimage)
add_subdirectory(thirdparty/libGPCGUIRenderer/lib)

add_subdirectory(lib)

# TODO: the following produces a (non-fatal) error complaining about the unavailability
# of a header file called "vertex.glsl.h", which is part of the dependency target 
# libGPCGUIGLCanvas but is a generated file.
add_subdirectory(testapp)
