cmake_minimum_required(VERSION 3.18)
project(
  rigel
  VERSION 0.0.1
  DESCRIPTION "Project Rigel"
  LANGUAGES CXX C)

#find_package( OpenGL REQUIRED )
#include_directories( ${OPENGL_INCLUDE_DIRS} )

add_subdirectory(vendor)

set(RIGEL_CPP_SOURCES
    "src/collider.cpp"
    "src/debug.cpp"
    "src/entity.cpp"
    "src/fs_linux.cpp"
    "src/game.cpp"
    "src/input_sdl.cpp"
    "src/json.cpp"
    "src/render.cpp"
    "src/resource.cpp"
    "src/tilemap.cpp"
    "src/trigger.cpp"
    "src/world.cpp")

set(RIGEL_MAIN "src/main_sdl.cpp")

file(GLOB RIGEL_TEST_SOURCES "tests/*.cpp")

# Compiler options (inherited by sub-folders)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall -Werror)
add_compile_options(-fdiagnostics-color)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(RIGEL_DEBUG)
endif()

add_executable(rigel ${RIGEL_CPP_SOURCES} ${RIGEL_MAIN})
target_link_libraries(rigel LINK_PRIVATE SDL3::SDL3 glad stb_image m)
target_compile_definitions(rigel PUBLIC DOCTEST_CONFIG_DISABLE)

# TODO(spencer): I'm basically building my application twice. There must be
# a better way. 
# 
# I don't really wanna have to restructure my project to be a static lib, and
# I _certainly_ don't want to integrate doctest into my main fn. "unintrusive" my butt!
add_executable(rigel_tests ${RIGEL_CPP_SOURCES} ${RIGEL_TEST_SOURCES})
target_include_directories(rigel_tests PUBLIC "src")
target_link_libraries(rigel_tests LINK_PRIVATE SDL3::SDL3 glad stb_image m)
