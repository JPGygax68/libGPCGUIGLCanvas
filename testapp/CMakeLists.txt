cmake_minimum_required(VERSION 3.0)

project(TestApp)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} GPC_GUIGLPainter)

# We need SDL2

find_library(SDL2_LIB SDL2)
if (NOT SDL2_LIB) 
    message(ERROR "Couldn't find SDL2 library")
endif()

add_library(SDL2 STATIC IMPORTED)
set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION ${SDL2_LIB})
find_path(SDL2_INCLUDE_DIR SDL2/SDL.h)
if (NOT SDL2_INCLUDE_DIR) 
    message(ERROR "Couldn't find SDL2 include directory")
endif()
set_target_properties(SDL2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIR})

find_library(SDL2_MAIN_LIB SDL2main)
if (NOT SDL2_LIB) 
    message(ERROR "Couldn't find SDL2 \"main\" object file")
endif()

add_library(SDL2_MAIN STATIC IMPORTED)
set_target_properties(SDL2_MAIN PROPERTIES IMPORTED_LOCATION ${SDL2_MAIN_LIB})

target_link_libraries(${PROJECT_NAME} SDL2 SDL2_MAIN)

# OpenGL

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR})