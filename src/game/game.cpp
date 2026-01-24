#include "game.h"
#include "../camera/camera.h"
#include "../character/character.h"
#include "../gui/filepicker.h"
#include "imgui.h"
#include <memory>
#include <raylib.h>
namespace moiras {
Game::Game() { auto root = std::make_unique<GameObject>(); }
void Game::setup() {
  // Create main camera
  auto mainCamera = std::make_unique<GameCamera>("MainCamera");
  auto *cameraPtr = mainCamera.get();

  rlImGuiSetup(true);
  // Create map
  std::unique_ptr<Map> map =
      // moiras::mapFromHeightmap("../assets/heightmap.png", 1000, 50, 1000);
      moiras::mapFromModel("../assets/map.glb");
  map->seaShaderFragment = ("../assets/shaders/sea_shader.fs");
  map->seaShaderVertex = ("../assets/shaders/sea_shader.vs");
  map->loadSeaShader();
  map->addSea();

  // Create initial character with model
  auto character = std::make_unique<Character>();
  auto sidebar = std::make_unique<Sidebar>();
  character->loadModel("../assets/knight_artorias_statue.glb");
  character->position = {0, 0, 0};
  // Snap to ground
  character->snapToGround(map->model);
  // Add everything to root
  root.addChild(std::move(mainCamera));
  root.addChild(std::move(map));
  root.addChild(std::move(character));
  root.addChild(std::move(sidebar));
}

void Game::loop(Window window) {
  while (!window.shouldClose()) // Detect window close button or ESC key
  {
    root.update();
    auto camera = root.getChildOfType<GameCamera>();
    auto mainchar = root.getChildOfType<Character>();
    auto map = root.getChildOfType<Map>();
    mainchar->name = "lolol";
    mainchar->snapToGround(map->model);
    
    camera->beginDrawing();
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
        break;
      }
    }
    if (closest.hit) {
      DrawCube(closest.point, 0.3f, 0.3f, 0.3f, ORANGE);
      Vector3 normalEnd;
      normalEnd.x = closest.point.x + closest.normal.x;
      normalEnd.y = closest.point.y + closest.normal.y;
      normalEnd.z = closest.point.z + closest.normal.z;
      DrawLine3D(closest.point, normalEnd, RED);
    }
    camera->endMode3D();
    
    // ImGui calls AFTER endMode3D but BEFORE endDrawing
    rlImGuiBegin();
    
    root.gui(); // This will call Sidebar::gui()
    
    bool open = true;
    ImGui::ShowDemoWindow(&open);
    if (ImGui::Begin("Test Window", &open)) {
      ImGui::TextUnformatted(ICON_FA_JEDI);
    }
    ImGui::End();
    
    rlImGuiEnd();
    
    camera->endDrawing();
  }
}
} // namespace moiras
