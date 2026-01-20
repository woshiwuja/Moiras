cmake_minimum_required(VERSION 4.1)
project(Moiras)

set(CMAKE_CXX_STANDARD 20)
add_custom_target(copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
)
add_custom_target(copy_steamid
        COMMAND ${CMAKE_COMMAND} -E copy steam_appid.txt ${CMAKE_CURRENT_BINARY_DIR}
)
add_subdirectory(src)
add_executable(
	Moiras
        src/main.cpp
	#src/clay.h
	#src/clay_renderer_raylib.c
	src/character/character.h
	src/character/character.cpp
	src/game/game.cpp
	src/game/game.h
	src/game/game_object.h
	src/game/game_object.cpp
)
target_link_libraries(Moiras PRIVATE raylib)
