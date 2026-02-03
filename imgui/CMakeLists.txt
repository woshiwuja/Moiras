cmake_minimum_required(VERSION 3.14)
project(imgui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Freetype for ImGui Freetype extension
find_package(Freetype REQUIRED)

# Source files - core imgui + Freetype extension from Ned's ImGui
set(IMGUI_SOURCES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../ned/lib/imgui/misc/freetype/imgui_freetype.cpp
)

add_library(imgui STATIC
    ${IMGUI_SOURCES}
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ned/lib/imgui/misc/freetype
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(imgui PUBLIC
    Freetype::Freetype
)