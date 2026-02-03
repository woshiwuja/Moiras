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

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(sol2)

# Find dependencies for Ned editor
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)
find_package(Freetype REQUIRED)
find_package(GLEW REQUIRED)

# Configure Ned build options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(USE_SSH OFF CACHE BOOL "" FORCE)

# Use Ned's modified ImGui instead of our basic one
# Build ImGui from Ned's lib directory with GLFW backend and Ned's config
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ned/lib/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_DIR}/misc/freetype
    ${CMAKE_CURRENT_SOURCE_DIR}/ned  # For ned_imgui_config.h
)
target_compile_definitions(imgui PUBLIC IMGUI_USER_CONFIG="ned_imgui_config.h")
target_link_libraries(imgui PUBLIC Freetype::Freetype glfw)

# Add Ned's ImGui directories globally so Ned can find them
include_directories(${IMGUI_DIR})
include_directories(${IMGUI_DIR}/backends)
include_directories(${IMGUI_DIR}/misc/freetype)

# Add Ned subdirectory (only build ned_embed library)
add_subdirectory(ned EXCLUDE_FROM_ALL)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
)

find_package(Lua 5.4 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})


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
    rlImGui/rlImGui.cpp
    src/gui/inventory.hpp
    src/gui/inventory.cpp
    rlImGui/rlImGui.h
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
)

target_include_directories(Moiras PUBLIC
    ${JoltPhysics_SOURCE_DIR}/..
    ${recastnavigation_SOURCE_DIR}/Recast/Include
    ${recastnavigation_SOURCE_DIR}/Detour/Include
    ${recastnavigation_SOURCE_DIR}/DetourCrowd/Include
    ${recastnavigation_SOURCE_DIR}/DetourTileCache/Include
    raylib/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
    ${CMAKE_CURRENT_SOURCE_DIR}/rlImGui/
)
target_include_directories(Moiras PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/rlImGui
    ${sol2_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ned
    ${CMAKE_CURRENT_SOURCE_DIR}/ned/lib/utfcpp/source
    ${CMAKE_CURRENT_SOURCE_DIR}/ned/lib/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/ned/lib/imgui/misc/freetype
)

# Disable Font Awesome in rlImGui (optional font icons)
target_compile_definitions(Moiras PRIVATE NO_FONT_AWESOME)

target_link_libraries(Moiras
    PRIVATE
    raylib
    Jolt
    imgui
    Recast
    Detour
    DetourCrowd
    DetourTileCache
    ${LUA_LIBRARIES}
    ned_embed
    GLEW::GLEW
)

# Allow multiple definitions for stb_image (used by both Raylib and Ned)
target_link_options(Moiras PRIVATE -Wl,--allow-multiple-definition)

# Copy scripts to build directory
add_custom_command(TARGET Moiras POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/scripts
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/../assets/scripts
)
