cmake_minimum_required(VERSION 3.14)
project(imgui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(IMGUI_SOURCES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
)

# Backend sources - aggiungi quelli che ti servono
set(IMGUI_BACKEND_SOURCES
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC
    ${IMGUI_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

# Se usi GLFW
find_package(glfw3 QUIET)
if(glfw3_FOUND)
    target_link_libraries(imgui PUBLIC glfw)
endif()

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(imgui PUBLIC OpenGL::GL)