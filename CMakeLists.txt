cmake_minimum_required(VERSION 4.1)
project(Moiras)

set(CMAKE_CXX_STANDARD 20)
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
	src/gui/sidebar.cpp
	rlImGui/rlImGui.cpp
	rlImGui/rlImGui.h
)
target_link_libraries(Moiras PRIVATE raylib)
target_link_libraries(Moiras PRIVATE imgui)
