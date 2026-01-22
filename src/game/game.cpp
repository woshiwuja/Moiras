#include "game.h"
#include "../camera/camera.h"
#include "../map/map.h"
#include <raylib.h>
namespace moiras {
Game::Game() { auto root = std::make_unique<GameObject>(); }
void Game::loop(Window window) {
  while (!window.shouldClose()) // Detect window close button or ESC key
  {
    root.update();
    auto camera = root.getChildOfType<GameCamera>();
    camera->beginDrawing();
    auto map = root.getChildOfType<Map>();
    camera->beginMode3D();
    root.draw();
    auto ray = camera->getRay();
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
    camera->endMode3D();
    root.gui();
    camera->endDrawing();
  }
};
} // namespace moiras
