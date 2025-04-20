cmake_minimum_required(VERSION 3.18)
project(
  rigel
  VERSION 0.0.1
  DESCRIPTION "Project Rigel"
  LANGUAGES CXX C)

#find_package( OpenGL REQUIRED )
#include_directories( ${OPENGL_INCLUDE_DIRS} )

add_subdirectory(vendor)

file(GLOB RIGEL_SOURCES "src/*.cpp")

# Compiler options (inherited by sub-folders)
set(CMAKE_CXX_STANDARD 20)
# TODO: put this back on please
#add_compile_options(-Wall -Werror -Wpedantic -Wextra)
add_compile_options(-fdiagnostics-color)

add_executable(rigel ${RIGEL_SOURCES})
# TODO: link the libs properly
target_link_libraries(rigel LINK_PRIVATE SDL3::SDL3 glad stb_image tinyxml2 m)
