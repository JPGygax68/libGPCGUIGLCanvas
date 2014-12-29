cmake_minimum_required(VERSION 3.0)

# Project name
project(libGPCGUIGLCanvas)

# Version (semantic: major, minor, patch-level)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

add_subdirectory(thirdparty/GLUtils)
add_subdirectory(thirdparty/libGPCGUICanvas/lib)
add_subdirectory(thirdparty/libGPCGUICanvas/testsuite)

add_subdirectory(lib)

add_subdirectory(testapp)
