#include "game.h"
#include "../building/structure.h"
#include "../camera/camera.h"
#include "../character/character.h"
#include "../gui/gui.h"
#include "../gui/sidebar.h"
#include "../gui/script_editor.h"
#include "../input/input_manager.h"
#include "../time/time_manager.h"
#include "../map/environment.hpp"
#include "../src/audio/audiodevice.hpp"
#include "../scripting/ScriptEngine.hpp"
#include "../scripting/ScriptComponent.hpp"
#include "imgui.h"
#include <cstdio>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <r3d/r3d.h>

namespace moiras
{

  Game::Game() { auto root = std::make_unique<GameObject>(); }

  void Game::setup()
  {
    rlImGuiSetup(false);

    // Initialize r3d rendering pipeline
    R3D_Init(GetScreenWidth(), GetScreenHeight());

    renderLoadingFrame("Inizializzazione scripting...", 0.0f);

    // Initialize the Lua scripting engine
    ScriptEngine::instance().initialize();
    ScriptEngine::instance().setGameRoot(&root);
    ScriptEngine::instance().setGame(this);
    ScriptEngine::instance().setScriptsDirectory("../assets/scripts");

    // Create main camera
    auto mainCamera = std::make_unique<GameCamera>("MainCamera");

    renderLoadingFrame("Caricamento mappa...", 0.10f);

    std::unique_ptr<Map> map = moiras::mapFromModel("../assets/map.glb");

    SetTextureFilter(map->model.materials[0].maps->texture,
                     TEXTURE_FILTER_ANISOTROPIC_8X);
    SetTextureFilter(map->model.materials->maps->texture,
                     TEXTURE_FILTER_ANISOTROPIC_8X);
    map->seaShaderFragment = ("../assets/shaders/sea_shader.fs");
    map->seaShaderVertex = ("../assets/shaders/sea_shader.vs");
    map->loadSeaShader();
    map->addSea();

    renderLoadingFrame("Inizializzazione interfaccia...", 0.20f);

    auto gui = std::make_unique<Gui>();
    gui->setModelManager(&modelManager);
    auto sidebar = gui->getChildOfType<Sidebar>();
    if (sidebar)
    {
      sidebar->lightManager = &lightmanager;
      sidebar->modelManager = &modelManager;
      TraceLog(LOG_INFO, "LightManager and ModelManager linked to Sidebar");
    }

    // Create script editor as child of GUI
    auto scriptEditorPtr = std::make_unique<ScriptEditor>();
    scriptEditor = scriptEditorPtr.get();
    scriptEditor->setOpen(false);
    gui->addChild(std::move(scriptEditorPtr));

    if (sidebar)
    {
      sidebar->scriptEditor = scriptEditor;
    }

    TraceLog(LOG_INFO, "Script Editor initialized (press F12 to open)");
    renderLoadingFrame("Caricamento audio...", 0.30f);

    auto audioManager = std::make_unique<AudioManager>();
    audioManager->setVolume(0.3);
    audioManager->loadMusicFolder("../assets/audio/music");
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

    renderLoadingFrame("Configurazione luci r3d...", 0.40f);

    // Setup r3d directional light (sun)
    r3dDirLight = R3D_CreateLight(R3D_LIGHT_DIR);
    R3D_SetLightDirection(r3dDirLight, (Vector3){-1.0f, -1.5f, -1.0f});
    R3D_SetLightColor(r3dDirLight, WHITE);
    R3D_SetLightActive(r3dDirLight, true);
    R3D_SetLightRange(r3dDirLight, 2000.0f);
    R3D_SetShadowSoftness(r3dDirLight, 1.5f);
    R3D_EnableShadow(r3dDirLight);

    // Setup r3d omni (point) light
    r3dPointLight = R3D_CreateLight(R3D_LIGHT_OMNI);
    R3D_SetLightPosition(r3dPointLight, (Vector3){0.0f, 50.0f, 0.0f});
    R3D_SetLightColor(r3dPointLight, (Color){255, 230, 200, 255});
    R3D_SetLightRange(r3dPointLight, 300.0f);
    R3D_SetLightActive(r3dPointLight, true);

    // Set ambient light
    R3D_ENVIRONMENT_SET(ambient.color, ((Color){40, 50, 70, 255}));
    R3D_ENVIRONMENT_SET(ambient.energy, 0.4f);

    // Create scene-graph light nodes (for hierarchy display and GUI)
    auto light1 = std::make_unique<DirectionalLight>("Light1");
    light1->position = {-100.0f, 150.0f, -100.0f};
    light1->target = {0.0f, 0.0f, 0.0f};
    light1->color = WHITE;
    light1->intensity = 1.0f;
    light1->enabled = true;

    auto light2 = std::make_unique<PointLight>("Light2");
    light2->position = {0.0f, 50.0f, 0.0f};
    light2->color = {255, 230, 200, 255};
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

    lightmanager.addLight(light1Ptr);
    lightmanager.addLight(light2Ptr);

    if (mapPtr)
    {
      renderLoadingFrame("Costruzione NavMesh...", 0.50f);
      mapPtr->buildNavMesh([this](int current, int total)
                           {
        float navProgress = (float)current / (float)total;
        float overallProgress = 0.50f + navProgress * 0.40f;
        char msg[128];
        snprintf(msg, sizeof(msg), "Costruzione NavMesh... (tile %d/%d)", current, total);
        renderLoadingFrame(msg, overallProgress); });
    }

    // Generate instanced rocks on the map
    if (mapPtr && mapPtr->model.meshCount > 0)
    {
      renderLoadingFrame("Generazione rocce...", 0.90f);
      auto rocks = std::make_unique<EnvironmentalObject>(1.0f, 200.0f);
      rocks->generate(mapPtr->model, 300, RockMeshType::CUBE);
      rocks->generate(mapPtr->model, 200, RockMeshType::SPHERE);
      auto *rocksPtr = rocks.get();
      root.addChild(std::move(rocks));
      if (sidebar)
      {
        sidebar->environmentObject = rocksPtr;
      }
      TraceLog(LOG_INFO, "Instanced rocks added to scene");
    }

    renderLoadingFrame("Caricamento personaggio...", 0.92f);
    auto player = std::make_unique<Character>();
    player->name = "Player";
    player->tag = "player";
    player->loadModel(modelManager, player->model_path);
    player->position = {0.0f, 10.0f, 0.0f};
    player->scale = 0.05f;
    registerObject(player->id, player.get());
    auto playerPtr = getObjectByID<Character>(player->id);
    root.addChild(std::move(player));

    if (mapPtr && playerPtr)
    {
      playerController = std::make_unique<CharacterController>(playerPtr, &mapPtr->navMesh, &mapPtr->model);
      playerController->setMovementSpeed(12.0f);
      TraceLog(LOG_INFO, "Player controller created and initialized");
    }

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
      TraceLog(LOG_INFO, "StructureBuilder configured with Map, NavMesh and ModelManager");
    }

    renderLoadingFrame("Pronto!", 1.0f);
    TraceLog(LOG_INFO, "SCRIPTING: Lua scripting system ready");
  }

  void Game::renderLoadingFrame(const char *message, float progress)
  {
    BeginDrawing();
    ClearBackground({30, 30, 40, 255});

    rlImGuiBegin();

    ImGuiIO &io = ImGui::GetIO();
    float windowWidth = 420.0f;
    float windowHeight = 100.0f;
    ImGui::SetNextWindowPos(
        ImVec2((io.DisplaySize.x - windowWidth) * 0.5f,
               (io.DisplaySize.y - windowHeight) * 0.5f),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

    ImGui::Begin("##Loading", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoCollapse);

    ImGui::Text("%s", message);
    ImGui::Spacing();
    ImGui::ProgressBar(progress, ImVec2(-1, 0));
    ImGui::Text("%d%%", (int)(progress * 100.0f));

    ImGui::End();
    rlImGuiEnd();

    EndDrawing();
  }

  void Game::updateScriptsRecursive(GameObject *obj, float dt)
  {
    if (!obj)
      return;
    if (auto *script = obj->getScriptComponent())
    {
      if (script->isLoaded() && !script->hasError())
      {
        script->onUpdate(dt);
      }
    }
    for (auto &child : obj->children)
    {
      if (child)
      {
        updateScriptsRecursive(child.get(), dt);
      }
    }
  }

  void Game::loop(Window window)
  {
    while (!window.shouldClose())
    {
      TimeManager::getInstance().update();

      InputManager &input = InputManager::getInstance();

      if (scriptEditor && scriptEditor->isOpen())
      {
        input.setContext(InputContext::UI);
      }
      else if (structureBuilder && structureBuilder->isBuildingMode())
      {
        input.setContext(InputContext::BUILDING);
      }
      else
      {
        input.setContext(InputContext::GAME);
      }

      input.update();

      if (input.isActionJustPressed(InputAction::UI_TOGGLE_PAUSE))
      {
        TimeManager::getInstance().togglePause();
      }
      if (input.isActionJustPressed(InputAction::UI_SPEED_NORMAL))
      {
        TimeManager::getInstance().setTimeScale(1.0f);
      }
      if (input.isActionJustPressed(InputAction::UI_SPEED_MEDIUM))
      {
        TimeManager::getInstance().setTimeScale(2.5f);
      }
      if (input.isActionJustPressed(InputAction::UI_SPEED_FAST))
      {
        TimeManager::getInstance().setTimeScale(5.0f);
      }
      if (input.isActionJustPressed(InputAction::UI_TOGGLE_SCRIPT_EDITOR) && scriptEditor)
      {
        scriptEditor->setOpen(!scriptEditor->isOpen());
      }

      root.update();

      m_frameCount++;
      if (m_frameCount % 60 == 0)
      {
        ScriptEngine::instance().hotReload();
      }

      float dt = TimeManager::getInstance().getGameDeltaTime();
      updateScriptsRecursive(&root, dt);

      auto camera = root.getChildOfType<GameCamera>();
      auto map = root.getChildOfType<Map>();
      auto *rocks = root.getChildOfType<EnvironmentalObject>();

      bool inBuildingMode = (structureBuilder && structureBuilder->isBuildingMode());
      bool inBrushMode = (rocks && rocks->isBrushMode());

      if (playerController && camera && !inBuildingMode && !inBrushMode)
      {
        playerController->update(camera);
      }

      if (rocks) {
        rocks->updateCameraPos(camera->rcamera.position);
      }

      // Raycast for cursor highlight (done outside 3D mode - just math)
      auto ray = camera->getRay();
      RayCollision closest = {0};
      closest.hit = false;
      closest.distance = std::numeric_limits<float>::max();

      if (map)
      {
        for (int m = 0; m < map->model.meshCount; m++)
        {
          RayCollision meshHitInfo = GetRayCollisionMesh(ray, map->model.meshes[m], map->model.transform);
          if (meshHitInfo.hit && meshHitInfo.distance < closest.distance)
          {
            closest = meshHitInfo;
            break;
          }
        }
      }

      // Update sea shader time uniform
      if (map && map->seaShaderLoaded.id > 0 && map->seaViewPosLoc >= 0)
      {
        float camPos[3] = {camera->rcamera.position.x, camera->rcamera.position.y, camera->rcamera.position.z};
        SetShaderValue(map->seaShaderLoaded, map->seaViewPosLoc, camPos, SHADER_UNIFORM_VEC3);
      }

      // --- Render frame ---
      camera->beginDrawing(); // BeginDrawing + ClearBackground + rlClearScreenBuffers

      // r3d 3D render pass: PBR terrain, characters, structures, rocks
      R3D_Begin(camera->rcamera);
      root.draw();
      R3D_End();

      // Post-r3d 3D pass: sea (custom shader) + debug overlays
      BeginMode3D(camera->rcamera);

      // Sea with custom shader (drawn after r3d composites the terrain)
      if (map)
      {
        map->drawSea();
      }

      // Cursor highlight
      if (closest.hit)
      {
        if (inBrushMode && rocks)
        {
          float brushR = rocks->getBrushRadius();
          DrawCircle3D(closest.point, brushR,
                       {1.0f, 0.0f, 0.0f}, 90.0f, {0, 200, 0, 180});

          if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
          {
            rocks->paintAt(closest.point);
          }
          if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
          {
            rocks->eraseAt(closest.point);
          }
        }
        else
        {
          DrawCube(closest.point, 0.3f, 0.3f, 0.3f, ORANGE);
          Vector3 normalEnd = Vector3Add(closest.point, closest.normal);
          DrawLine3D(closest.point, normalEnd, RED);
        }
      }

      // NavMesh debug
      if (map && map->showNavMeshDebug && map->navMeshBuilt)
      {
        map->navMesh.drawDebug();
      }

      // Path visualization
      if (map && map->showPath && map->debugPath.size() > 1)
      {
        for (size_t i = 0; i < map->debugPath.size() - 1; i++)
        {
          DrawLine3D(map->debugPath[i], map->debugPath[i + 1], RED);
          DrawSphere(map->debugPath[i], 0.5f, YELLOW);
        }
        DrawSphere(map->debugPath.back(), 0.5f, YELLOW);
      }

      if (playerController && map && map->showPath)
      {
        playerController->drawDebug();
      }

      EndMode3D();

      // ImGui overlay
      rlImGuiBegin();
      root.gui();
      rlImGuiEnd();

      camera->endDrawing();
    }
  }

  Game::~Game()
  {
    ScriptEngine::instance().shutdown();
    if (r3dDirLight) R3D_DestroyLight(r3dDirLight);
    if (r3dPointLight) R3D_DestroyLight(r3dPointLight);
    R3D_Close();
    rlImGuiShutdown();
  }
} // namespace moiras
