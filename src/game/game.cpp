#include "game.h"
#include "../camera/camera.h"
namespace moiras {
Game::Game() {
  auto root = std::make_unique<GameObject>();

}
void Game::loop(Window window){
  while (!window.shouldClose()) // Detect window close button or ESC key
  {
    root.update();
    auto camera = root.getChildOfType<GameCamera>();
    camera->beginDrawing();
    camera->beginMode3D();
    root.draw();
    camera->endMode3D();
    camera->endDrawing();
  }
};
} // namespace moiras
