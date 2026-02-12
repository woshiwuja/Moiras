cmake_minimum_required(VERSION 3.5...4.1)
project(Moiras)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Allow older CMake versions in subdirectories
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

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

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Setup Filament
set(FILAMENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/filament")
if(NOT EXISTS ${FILAMENT_DIR})
    message(FATAL_ERROR "Filament not found at ${FILAMENT_DIR}. Please run: \n"
            "cd external/filament && wget https://github.com/google/filament/releases/download/v1.52.0/filament-v1.52.0-linux.tgz && "
            "tar -xzf filament-v1.52.0-linux.tgz --strip-components=1")
endif()
include_directories(${FILAMENT_DIR}/include)
link_directories(${FILAMENT_DIR}/lib/x86_64)

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

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(sol2)

# Basic ImGui with SDL2 + OpenGL3 backends
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)

# Add ImGuiColorTextEdit
set(IMGUICOLORTEXTEDIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuiColorTextEdit)
add_library(ImGuiColorTextEdit STATIC
    ${IMGUICOLORTEXTEDIT_DIR}/TextEditor.cpp
)
target_include_directories(ImGuiColorTextEdit PUBLIC 
    ${IMGUICOLORTEXTEDIT_DIR}
    ${IMGUI_DIR}
)
target_link_libraries(ImGuiColorTextEdit PUBLIC imgui)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
)

find_package(Lua 5.4 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})


add_subdirectory(src)

add_executable(
    Moiras
    src/main.cpp
    src/commons/math_types.h
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
    src/audio/audiodevice.hpp
    src/audio/audiodevice.cpp
    src/navigation/navmesh.h
    src/navigation/navmesh.cpp
    src/gui/sidebar.cpp
    src/building/structure.h
    src/building/structure.cpp
    src/building/structure_builder.h
    src/building/structure_builder.cpp
    src/resources/model_manager.h
    src/resources/model_manager.cpp
    src/gui/inventory.hpp
    src/gui/inventory.cpp
    # SDL + Filament rendering
    src/window/sdl_window.h
    src/window/sdl_window.cpp
    src/rendering/filament_engine.h
    src/rendering/filament_engine.cpp
    # Lua scripting system
    src/scripting/ScriptEngine.hpp
    src/scripting/ScriptEngine.cpp
    src/scripting/ScriptComponent.hpp
    src/scripting/ScriptComponent.cpp
    src/scripting/LuaBindings.hpp
    src/scripting/LuaBindings.cpp
    src/scripting/bindings/MathBindings.hpp
    src/scripting/bindings/MathBindings.cpp
    src/scripting/bindings/InputBindings.hpp
    src/scripting/bindings/InputBindings.cpp
    src/scripting/bindings/WorldBindings.hpp
    src/scripting/bindings/WorldBindings.cpp
    src/scripting/bindings/GameObjectBindings.hpp
    src/scripting/bindings/GameObjectBindings.cpp
    src/scripting/bindings/CharacterBindings.hpp
    src/scripting/bindings/CharacterBindings.cpp
    # Script Editor
    src/gui/script_editor.h
    src/gui/script_editor.cpp
    src/input/input_manager.cpp
    src/input/input_manager.h
    src/input/input_types.h
    # Time management
    src/time/time_manager.cpp
    src/time/time_manager.h
    # Environment (GPU instanced rocks)
    src/map/environment.hpp
    src/map/environment.cpp
)

target_include_directories(Moiras PUBLIC
    ${JoltPhysics_SOURCE_DIR}/..
    ${recastnavigation_SOURCE_DIR}/Recast/Include
    ${recastnavigation_SOURCE_DIR}/Detour/Include
    ${recastnavigation_SOURCE_DIR}/DetourCrowd/Include
    ${recastnavigation_SOURCE_DIR}/DetourTileCache/Include
    ${FILAMENT_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
    ${CMAKE_CURRENT_SOURCE_DIR}/include/raylib  # Raylib headers for type compatibility
)
target_include_directories(Moiras PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${sol2_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuiColorTextEdit
    ${SDL2_INCLUDE_DIRS}
)

# Material compilation for Filament
set(MATC ${FILAMENT_DIR}/bin/matc)
set(MAT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/materials)
set(MAT_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/materials)

add_custom_command(
    OUTPUT ${MAT_OUT_DIR}/pbr.filamat
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MAT_OUT_DIR}
    COMMAND ${MATC} -a opengl -p mobile
            -o ${MAT_OUT_DIR}/pbr.filamat
            ${MAT_SRC_DIR}/pbr.mat
    DEPENDS ${MAT_SRC_DIR}/pbr.mat
    COMMENT "Compiling PBR material"
)

add_custom_target(compile_materials ALL
    DEPENDS ${MAT_OUT_DIR}/pbr.filamat
)

add_dependencies(Moiras compile_materials)

target_link_libraries(Moiras
    PRIVATE
    ${SDL2_LIBRARIES}
    # Filament libraries (order matters!)
    filament
    filamat
    gltfio_core
    backend
    utils
    # Keep existing
    Jolt
    imgui
    ImGuiColorTextEdit
    Recast
    Detour
    DetourCrowd
    DetourTileCache
    ${LUA_LIBRARIES}
    # System libraries needed by Filament
    GL
    pthread
    dl
)

# Copy scripts to build directory
add_custom_command(TARGET Moiras POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/scripts
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/../assets/scripts
)
