#include "game.h"
#include "../camera/camera.h"
#include "../character/character.h"
#include "../gui/gui.h"
#include "imgui.h"
#include <memory>
#include <raylib.h>
namespace moiras {
Game::Game() { auto root = std::make_unique<GameObject>(); }
void Game::setup() {
  // Create main camera
  auto mainCamera = std::make_unique<GameCamera>("MainCamera");
  auto *cameraPtr = mainCamera.get();
  renderTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  rlImGuiSetup(false);
  // Create map
  std::unique_ptr<Map> map =
      // moiras::mapFromHeightmap("../assets/heightmap.png", 1000, 50, 1000);
      moiras::mapFromModel("../assets/map.glb");

  SetTextureFilter(map->model.materials[0].maps->texture,
                   TEXTURE_FILTER_BILINEAR);
  map->seaShaderFragment = ("../assets/shaders/sea_shader.fs");
  map->seaShaderVertex = ("../assets/shaders/sea_shader.vs");
  map->loadSeaShader();
  map->addSea();

  auto character = std::make_unique<Character>();
  character->loadModel("../assets/knight_artorias_statue.glb");
  character->position = {0, 0, 0};
  character->snapToGround(map->model);

  auto gui = std::make_unique<Gui>();
  root.addChild(std::move(mainCamera));
  root.addChild(std::move(map));
  root.addChild(std::move(character));
  root.addChild(std::move(gui));
  rlSetClipPlanes(0.5, 50000);
  outlineShader = LoadShader(0, "../assets/shaders/outline.fs");
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  int resolutionLoc = GetShaderLocation(outlineShader, "resolution");
  SetShaderValue(outlineShader, resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
}

void Game::loop(Window window) {
  while (!window.shouldClose()) // Detect window close button or ESC key
  {
    root.update();
    auto camera = root.getChildOfType<GameCamera>();
    auto map = root.getChildOfType<Map>();

    // Render to texture
    BeginTextureMode(renderTarget);
    ClearBackground(RAYWHITE);

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
    EndTextureMode();
    camera->beginDrawing();

    BeginShaderMode(outlineShader);
    DrawTextureRec(renderTarget.texture,
                   (Rectangle){0, 0, (float)renderTarget.texture.width,
                               (float)-renderTarget.texture.height},
                   (Vector2){0, 0}, WHITE);
    EndShaderMode();

    rlImGuiBegin();
    ImGui::ShowDemoWindow();
    root.gui();

    rlImGuiEnd();

    camera->endDrawing();
  }
}
} // namespace moiras
