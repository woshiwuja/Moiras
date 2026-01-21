#include "camera/camera.h"
#include "character/character.h"
#include "game/game.h"
#include "map/map.h"
#include "raylib.h"
#include "rcamera.h"
#include <memory>
const int screenWidth = 1920;
const int screenHeight = 1080;
string title = "Moiras";

using namespace moiras;
int main(void) {
  Game game = Game();
  Window window = Window(screenWidth, screenHeight, title, KEY_END, 144, true);
  window.init();
  auto character = std::make_unique<Character>();
  auto mainCamera = std::make_unique<GameCamera>("MainCamera");
  auto *cameraPtr = mainCamera.get();
  std::unique_ptr<Map> map =
      // moiras::mapFromHeightmap("../assets/heightmap.png", 1000, 50, 1000);
      moiras::mapFromModel("../assets/map.glb");
  map->seaShaderFragment = ("../assets/shaders/sea_shader.fs");
  map->seaShaderVertex = ("../assets/shaders/sea_shader.vs");
  map->loadSeaShader();
  map->addSea();
  game.root.addChild(std::move(mainCamera));
  game.root.addChild(std::move(map));
  game.root.addChild(std::move(character));
  game.loop(window);
  return 0;
}
