#include "camera/camera.h"
#include "character/character.h"
#include "game/game.h"
#include "raylib.h"
#include "rcamera.h"
#include "window/window.h"
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
  auto mainCamera = std::make_unique<GameCamera>("main camera");
  GameCamera *cameraPtr = mainCamera.get();
  game.root.addChild(std::move(mainCamera));
  game.root.addChild(std::move(character));
  while (!window.shouldClose()) // Detect window close button or ESC key
  {
    game.root.update();
    cameraPtr->beginDrawing();
    cameraPtr->beginMode3D();
    game.root.draw();
    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){600.0f, 600.0f},
              LIGHTGRAY); // Draw ground
    cameraPtr->endMode3D();
    cameraPtr->endDrawing();
  }
  return 0;
}
