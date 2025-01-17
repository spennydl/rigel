set(GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only")
set(GLAD_GL "")
add_subdirectory(glfw-3.4)

add_subdirectory(glad)
add_subdirectory(stb_image)
add_subdirectory(tinyxml2)
