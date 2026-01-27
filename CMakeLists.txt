cmake_minimum_required(VERSION 3.1)
project(Moiras)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

set(GENERATE_DEBUG_SYMBOLS ON)
# Number of bits to use in ObjectLayer. Can be 16 or 32.
set(OBJECT_LAYER_BITS 16)

# Select X86 processor features to use
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)
set(CMAKE_CXX_STANDARD 20)

# Fetch Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Configura GLFW (nuovo metodo)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

# Fetch JoltPhysics
FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
    GIT_TAG "v5.5.0"
    SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(JoltPhysics)

# Fetch RecastNavigation
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    RecastNavigation
    GIT_REPOSITORY https://github.com/recastnavigation/recastnavigation
    GIT_TAG v1.6.0
)
FetchContent_MakeAvailable(RecastNavigation)

# Crea libreria ImGui PRIMA dell'eseguibile
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC imgui/)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
)

add_subdirectory(src)

add_executable(
    Moiras
    src/main.cpp
    src/character/character.h
    src/character/character.cpp
    src/character/controller.h
    src/character/controller.cpp
    src/game/game.cpp
    src/camera/camera.cpp
    src/camera/camera.h
    src/game/game.h
    src/gui/gui.cpp
    src/gui/gui.h
    src/map/map.cpp
    src/map/map.h
    src/models/models.cpp
    src/models/models.h
    src/game/game_object.h
    src/game/game_object.cpp
    src/gui/sidebar.h
    src/lights/lightmanager.h
    src/lights/lightmanager.cpp
    src/lights/lights.cpp
    src/lights/lights.h
    src/navigation/navmesh.h
    src/navigation/navmesh.cpp
    src/gui/sidebar.cpp
    src/building/structure.h
    src/building/structure.cpp
    src/building/structure_builder.h
    src/building/structure_builder.cpp
    rlImGui/rlImGui.cpp
    rlImGui/rlImGui.h
)

target_include_directories(Moiras PUBLIC
    ${JoltPhysics_SOURCE_DIR}/..
    ${recastnavigation_SOURCE_DIR}/Recast/Include
    ${recastnavigation_SOURCE_DIR}/Detour/Include
    ${recastnavigation_SOURCE_DIR}/DetourCrowd/Include
    raylib/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
)
target_include_directories(Moiras PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)
target_link_libraries(Moiras
    PRIVATE
    raylib
    Jolt
    imgui
    Recast
    Detour
    DetourCrowd
)