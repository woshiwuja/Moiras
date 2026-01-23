#include "game.h"
#include "../camera/camera.h"
#include "../character/character.h"
#include <raylib.h>
namespace moiras {
Game::Game() { auto root = std::make_unique<GameObject>(); }
void Game::setup() {
  auto character = std::make_unique<Character>();
  character->loadModel("../assets/knight_artorias_statue.glb");
  auto mainCamera = std::make_unique<GameCamera>("MainCamera");
  auto *cameraPtr = mainCamera.get();
  std::unique_ptr<Map> map =
      // moiras::mapFromHeightmap("../assets/heightmap.png", 1000, 50, 1000);
      moiras::mapFromModel("../assets/map.glb");
  map->seaShaderFragment = ("../assets/shaders/sea_shader.fs");
  map->seaShaderVertex = ("../assets/shaders/sea_shader.vs");
  map->loadSeaShader();
  map->addSea();
  root.addChild(std::move(mainCamera));
  root.addChild(std::move(map));
  root.addChild(std::move(character));
}
void Game::loop(Window window) {
  while (!window.shouldClose()) // Detect window close button or ESC key
  {
    root.update();
    auto camera = root.getChildOfType<GameCamera>();
    auto mainchar = root.getChildOfType<Character>();
    mainchar->name = "lolol";
    camera->beginDrawing();
    auto map = root.getChildOfType<Map>();
    mainchar->snapToGround(map->model);
    camera->beginMode3D();
    root.draw();
    auto ray = camera->getRay();
    ///
    RayCollision closest = {0};
    closest.hit = false;
    closest.distance = std::numeric_limits<float>::max();
    RayCollision meshHitInfo = {0};
    for (int m = 0; m < map->model.meshCount; m++) {
      meshHitInfo =
          GetRayCollisionMesh(ray, map->model.meshes[m], map->model.transform);
      if (meshHitInfo.hit) {
        if ((!closest.hit) || (closest.distance > meshHitInfo.distance))
          closest = meshHitInfo;
        break; // Stop once one mesh collision is detected, the colliding mesh
               // is m
      }
    }

    if (closest.hit) {
      ORANGE;
      {
        DrawCube(closest.point, 0.3f, 0.3f, 0.3f, ORANGE);
        Vector3 normalEnd;
        normalEnd.x = closest.point.x + closest.normal.x;
        normalEnd.y = closest.point.y + closest.normal.y;
        normalEnd.z = closest.point.z + closest.normal.z;

        DrawLine3D(closest.point, normalEnd, RED);
      }
    }
    ///
    camera->endMode3D();
    root.gui();
    camera->endDrawing();
  }
};
} // namespace moiras
