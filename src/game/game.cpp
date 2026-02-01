#include "game.h"
#include "../building/structure.h"
#include "../camera/camera.h"
#include "../character/character.h"
#include "../gui/gui.h"
#include "../gui/sidebar.h"
#include "../src/audio/audiodevice.hpp"
#include "imgui.h"
#include <memory>
#include <raylib.h>
#include <rlgl.h>
namespace moiras
{

  Game::Game() { auto root = std::make_unique<GameObject>(); }

  void Game::setup()
  {
    // Create main camera
    auto mainCamera = std::make_unique<GameCamera>("MainCamera");

    renderTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    rlImGuiSetup(false);

    std::unique_ptr<Map> map = moiras::mapFromModel("../assets/map.glb");

    SetTextureFilter(map->model.materials[0].maps->texture,
                     TEXTURE_FILTER_ANISOTROPIC_8X);
    SetTextureFilter(map->model.materials->maps->texture,
                     TEXTURE_FILTER_ANISOTROPIC_8X);
    map->seaShaderFragment = ("../assets/shaders/sea_shader.fs");
    map->seaShaderVertex = ("../assets/shaders/sea_shader.vs");
    map->loadSeaShader();
    map->addSea();

    auto gui = std::make_unique<Gui>();
    gui->setModelManager(&modelManager);
    auto sidebar = gui->getChildOfType<Sidebar>();
    if (sidebar)
    {
      sidebar->lightManager = &lightmanager;
      sidebar->modelManager = &modelManager;
      TraceLog(LOG_INFO, "LightManager and ModelManager linked to Sidebar");
    }
    auto audioManager = std::make_unique<AudioManager>();
    audioManager->setVolume(0.3);
    audioManager->loadMusicFolder("../assets/audio/music");
    //audioManager->loadMusic("../assets/audio/music/Desert.mp3");
    audioManager->playMusic("Desert");
    registerObject(audioManager->id, audioManager.get());
    root.addChild(std::move(audioManager));
    auto structureBuilderPtr = std::make_unique<StructureBuilder>();
    registerObject(structureBuilderPtr->id, structureBuilderPtr.get());
    structureBuilder = structureBuilderPtr.get();
    root.addChild(std::move(structureBuilderPtr));
    if (sidebar)
    {
      sidebar->structureBuilder = structureBuilder;
      TraceLog(LOG_INFO, "StructureBuilder linked to Sidebar");
    }
    nearPlane = 0.5f;
    farPlane = 50000.0f;
    rlSetClipPlanes(nearPlane, farPlane);
    outlineShader = LoadShader(0, "../assets/shaders/outline.fs");
    float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
    SetShaderValue(outlineShader, GetShaderLocation(outlineShader, "resolution"),
                   resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(outlineShader, GetShaderLocation(outlineShader, "nearPlane"),
                   &nearPlane, SHADER_UNIFORM_FLOAT);
    SetShaderValue(outlineShader, GetShaderLocation(outlineShader, "farPlane"),
                   &farPlane, SHADER_UNIFORM_FLOAT);
    depthTextureLoc = GetShaderLocation(outlineShader, "depthTexture");
    celShader = LoadShader("../assets/shaders/cel_shading.vs", "../assets/shaders/cel_shading.fs");
    lightmanager = LightManager();
    lightmanager.loadShader("../assets/shaders/pbr.vs",
                            "../assets/shaders/pbr.fs");
    auto light1 = std::make_unique<DirectionalLight>("Light1");
    light1->position = {100.0f, 100.0f, 100.0f};
    light1->target = {0.0f, 0.0f, 0.0f};
    light1->color = WHITE;
    light1->intensity = 1.0f;
    light1->enabled = true;
    auto light2 = std::make_unique<PointLight>("Light2");
    light2->position = {0.0f, 50.0f, 0.0f};
    light2->color = WHITE;
    light2->intensity = 50.0f;
    light2->enabled = true;
    Light *light1Ptr = light1.get();
    Light *light2Ptr = light2.get();
    auto lights = std::make_unique<GameObject>("Lights");
    lights->addChild(std::move(light1));
    lights->addChild(std::move(light2));
    root.addChild(std::move(mainCamera));
    root.addChild(std::move(map));
    root.addChild(std::move(gui));
    root.addChild(std::move(lights));

    auto mapPtr = root.getChildOfType<Map>();
    if (mapPtr)
    {
      for (int i = 0; i < mapPtr->model.materialCount; i++)
      {
        mapPtr->model.materials[i].shader = lightmanager.getShader();
      }

      mapPtr->buildNavMesh();
      TraceLog(LOG_INFO, "Shader assigned, ID: %d",
               mapPtr->model.materials[0].shader.id);
    }

    lightmanager.addLight(light1Ptr);
    lightmanager.addLight(light2Ptr);

    Character::setSharedShader(celShader);
    TraceLog(LOG_INFO, "Added %d lights to manager", 2);
    auto player = std::make_unique<Character>();
    player->name = "Player";
    player->loadModel(modelManager, player->model_path);
    player->position = {0.0f, 10.0f, 0.0f}; // Posizione iniziale
    player->scale = .05f;
    registerObject(player->id, player.get());
    auto playerPtr = getObjectByID<Character>(player->id);
    root.addChild(std::move(player));
    if (mapPtr && playerPtr)
    {
      playerController = std::make_unique<CharacterController>(playerPtr, &mapPtr->navMesh, &mapPtr->model);
      playerController->setMovementSpeed(12.0f);
      TraceLog(LOG_INFO, "Player controller created and initialized");
    }

    // Configura lo StructureBuilder con le dipendenze
    auto cameraPtr = root.getChildOfType<GameCamera>();
    if (structureBuilder && mapPtr)
    {
      structureBuilder->setMap(mapPtr);
      structureBuilder->setNavMesh(&mapPtr->navMesh);
      structureBuilder->setModelManager(&modelManager);
      if (cameraPtr)
      {
        structureBuilder->setCamera(&cameraPtr->rcamera);
      }
      TraceLog(LOG_INFO,
               "StructureBuilder configured with Map, NavMesh and ModelManager");
    }

    // Imposta lo shader condiviso per le strutture
    Structure::setSharedShader(celShader);
  }

  void Game::loop(Window window)
  {
    while (!window.shouldClose())
    {
      root.update();

      auto camera = root.getChildOfType<GameCamera>();
      auto map = root.getChildOfType<Map>();

      // Aggiorna il player controller (solo se non siamo in building mode)
      bool inBuildingMode =
          (structureBuilder && structureBuilder->isBuildingMode());
      if (playerController && camera && !inBuildingMode)
      {
        playerController->update(camera);
      }
      // auto audioManager = root.getChildOfType<AudioManager>();
      //  Aggiorna luci
      lightmanager.updateCameraPosition(camera->rcamera.position);
      lightmanager.updateAllLights();

      // Render scene to texture
      BeginTextureMode(renderTarget);
      ClearBackground(DARKBLUE);
      camera->beginMode3D();
      root.draw();
      auto ray = camera->getRay();
      RayCollision closest = {0};
      closest.hit = false;
      closest.distance = std::numeric_limits<float>::max();
      RayCollision meshHitInfo = {0};

      for (int m = 0; m < map->model.meshCount; m++)
      {
        meshHitInfo =
            GetRayCollisionMesh(ray, map->model.meshes[m], map->model.transform);
        if (meshHitInfo.hit)
        {
          if ((!closest.hit) || (closest.distance > meshHitInfo.distance))
            closest = meshHitInfo;
          break;
        }
      }

      if (closest.hit)
      {
        DrawCube(closest.point, 0.3f, 0.3f, 0.3f, ORANGE);
        Vector3 normalEnd;
        normalEnd.x = closest.point.x + closest.normal.x;
        normalEnd.y = closest.point.y + closest.normal.y;
        normalEnd.z = closest.point.z + closest.normal.z;
        DrawLine3D(closest.point, normalEnd, RED);
      }

      camera->endMode3D();
      EndTextureMode();

      // Draw to screen SENZA outline shader per ora
      camera->beginDrawing();

      BeginShaderMode(outlineShader);
      DrawTextureRec(renderTarget.texture,
                     (Rectangle){0, 0, (float)renderTarget.texture.width,
                                 (float)-renderTarget.texture.height},
                     (Vector2){0, 0}, WHITE);
      EndShaderMode();

      camera->beginMode3D();
      if (map && map->showNavMeshDebug && map->navMeshBuilt)
      {
        map->navMesh.drawDebug();
      }
      if (map && map->showPath && map->debugPath.size() > 1)
      {
        for (size_t i = 0; i < map->debugPath.size() - 1; i++)
        {
          DrawLine3D(map->debugPath[i], map->debugPath[i + 1], RED);
          DrawSphere(map->debugPath[i], 0.5f, YELLOW);
        }
        DrawSphere(map->debugPath.back(), 0.5f, YELLOW);
      }

      // Draw player controller debug (path, waypoints, target)
      // Visibile sempre (non legato a showNavMeshDebug)
      if (playerController && map && map->showPath)
      {
        playerController->drawDebug();
      }

      camera->endMode3D();
      //  ImGui
      rlImGuiBegin();
      root.gui();
      rlImGuiEnd();

      camera->endDrawing();
    }
  }

  Game::~Game()
  {
    UnloadShader(outlineShader);
    UnloadRenderTexture(renderTarget);
    rlImGuiShutdown();
  }
} // namespace moiras
