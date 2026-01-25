cmake_minimum_required(VERSION 4.1)
project(Moiras)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

set(GENERATE_DEBUG_SYMBOLS ON)
# Number of bits to use in ObjectLayer. Can be 16 or 32.
set(OBJECT_LAYER_BITS 16)

# Select X86 processor features to use, by default the library compiles with AVX2, if everything is off it will be SSE2 compatible.
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

FetchContent_Declare(
								JoltPhysics
								GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
								GIT_TAG "v5.5.0"
		SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(JoltPhysics)
FetchContent_Declare(
				RecastNavigation
				GIT_REPOSITORY https://github.com/recastnavigation/recastnavigation
				GIT_TAG        v1.6.0  # O la versione più recente
)
FetchContent_MakeAvailable(RecastNavigation)
add_custom_target(copy_assets
								COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
)

add_subdirectory(src)
add_subdirectory(raylib-cpp)
add_executable(
	Moiras
	src/main.cpp
	#src/clay.h
	#src/clay_renderer_raylib.c
	src/character/character.h
	src/character/character.cpp
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
	raygui/src/raygui.h
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
	rlImGui/rlImGui.cpp
	rlImGui/rlImGui.h
)

target_include_directories(Moiras PUBLIC
				${JoltPhysics_SOURCE_DIR}/..
				${recastnavigation_SOURCE_DIR}/Recast/Include
				${recastnavigation_SOURCE_DIR}/Detour/Include
				${recastnavigation_SOURCE_DIR}/DetourCrowd/Include
)
target_link_libraries(Moiras
				PRIVATE
				raylib
				Jolt
				imgui
				Recast          # Generazione della NavMesh
				Detour          # Pathfinding sulla NavMesh
				DetourCrowd     # (Opzionale) Gestione agenti e ostacoli dinamici
)
