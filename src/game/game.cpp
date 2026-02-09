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
namespace moiras
{

  Game::Game() { auto root = std::make_unique<GameObject>(); }

  void Game::setup()
  {
    // Create render target and ImGui first so we can show loading progress
    renderTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    rlImGuiSetup(false);

    renderLoadingFrame("Inizializzazione scripting...", 0.0f);

    // Initialize the Lua scripting engine
    ScriptEngine::instance().initialize();
    ScriptEngine::instance().setGameRoot(&root);
    ScriptEngine::instance().setGame(this);
    ScriptEngine::instance().setScriptsDirectory("../assets/scripts");

    // Create main camera
    auto mainCamera = std::make_unique<GameCamera>("MainCamera");

    renderLoadingFrame("Caricamento mappa...", 0.10f);

    std::unique_ptr<Map> map = moiras::mapFromModel("../assets/map2.glb");

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
      sidebar->outlineEnabled = &this->outlineEnabled;
      TraceLog(LOG_INFO, "LightManager and ModelManager linked to Sidebar");
    }
    
    // Create script editor as child of GUI
    auto scriptEditorPtr = std::make_unique<ScriptEditor>();
    scriptEditor = scriptEditorPtr.get();
    scriptEditor->setOpen(false); // Start closed, can be toggled with F12
    gui->addChild(std::move(scriptEditorPtr));
    
    // Link script editor to sidebar
    if (sidebar) {
      sidebar->scriptEditor = scriptEditor;
    }
    
    TraceLog(LOG_INFO, "Script Editor initialized (press F12 to open)");
    renderLoadingFrame("Caricamento audio...", 0.30f);

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
    renderLoadingFrame("Compilazione shaders...", 0.40f);

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

      renderLoadingFrame("Costruzione NavMesh...", 0.50f);
      mapPtr->buildNavMesh([this](int current, int total) {
        float navProgress = (float)current / (float)total;
        float overallProgress = 0.50f + navProgress * 0.40f;
        char msg[128];
        snprintf(msg, sizeof(msg), "Costruzione NavMesh... (tile %d/%d)", current, total);
        renderLoadingFrame(msg, overallProgress);
      });
      TraceLog(LOG_INFO, "Shader assigned, ID: %d",
               mapPtr->model.materials[0].shader.id);
    }
    lightmanager.addLight(light1Ptr);
    lightmanager.addLight(light2Ptr);
    Character::setSharedShader(celShader);
    TraceLog(LOG_INFO, "Added %d lights to manager", 2);

    // Genera rocce instanziate sulla mappa
    if (mapPtr && mapPtr->model.meshCount > 0) {
      renderLoadingFrame("Generazione rocce...", 0.90f);
      auto rocks = std::make_unique<EnvironmentalObject>(500, 1.0f, 200.0f);
      rocks->generate(mapPtr->model);
      rocks->setShader(lightmanager.getShader());
      auto *rocksPtr = rocks.get();
      root.addChild(std::move(rocks));
      if (sidebar) {
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
      TraceLog(LOG_INFO,
               "StructureBuilder configured with Map, NavMesh and ModelManager");
    }

    Structure::setSharedShader(celShader);
    renderLoadingFrame("Inizializzazione ombre...", 0.96f);
    lightmanager.registerShadowShader(celShader);
    lightmanager.setupShadowMap("../assets/shaders/shadow_depth.vs",
                                 "../assets/shaders/shadow_depth.fs");
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
      // Update TimeManager first (caches delta time for the frame)
      TimeManager::getInstance().update();
      
      // Update input manager and set context based on game state
      InputManager& input = InputManager::getInstance();
      
      // Determine current context
      if (scriptEditor && scriptEditor->isOpen()) {
        input.setContext(InputContext::UI);
      } else if (structureBuilder && structureBuilder->isBuildingMode()) {
        input.setContext(InputContext::BUILDING);
      } else {
        input.setContext(InputContext::GAME);
      }
      
      // Update input state (must be called before any input queries)
      input.update();
      
      // Handle pause toggle (works in any context)
      if (input.isActionJustPressed(InputAction::UI_TOGGLE_PAUSE)) {
        TimeManager::getInstance().togglePause();
      }
      
      // Handle speed changes (works in any context)
      if (input.isActionJustPressed(InputAction::UI_SPEED_NORMAL)) {
        TimeManager::getInstance().setTimeScale(1.0f);
      }
      if (input.isActionJustPressed(InputAction::UI_SPEED_MEDIUM)) {
        TimeManager::getInstance().setTimeScale(2.5f);
      }
      if (input.isActionJustPressed(InputAction::UI_SPEED_FAST)) {
        TimeManager::getInstance().setTimeScale(5.0f);
      }
      
      // Toggle script editor with F12 (always works)
      if (input.isActionJustPressed(InputAction::UI_TOGGLE_SCRIPT_EDITOR) && scriptEditor)
      {
        scriptEditor->setOpen(!scriptEditor->isOpen());
      }
      
      root.update();

      // Hot-reload check every 60 frames (~1 second at 60fps)
      m_frameCount++;
      if (m_frameCount % 60 == 0)
      {
        ScriptEngine::instance().hotReload();
      }

      // Update all Lua scripts with scaled delta time
      float dt = TimeManager::getInstance().getGameDeltaTime();
      updateScriptsRecursive(&root, dt);

      auto camera = root.getChildOfType<GameCamera>();
      auto map = root.getChildOfType<Map>();

      // Aggiorna il player controller (solo se non siamo in building mode o brush mode)
      auto *rocks = root.getChildOfType<EnvironmentalObject>();
      bool inBuildingMode =
          (structureBuilder && structureBuilder->isBuildingMode());
      bool inBrushMode = (rocks && rocks->isBrushMode());
      if (playerController && camera && !inBuildingMode && !inBrushMode)
      {
        playerController->update(camera);
      }
      // auto audioManager = root.getChildOfType<AudioManager>();
      //  Aggiorna luci
      lightmanager.updateCameraPosition(camera->rcamera.position);
      lightmanager.updateAllLights();

      // Update cel shader uniforms needed for CSM shadow mapping
      {
        float camPos[3] = {camera->rcamera.position.x, camera->rcamera.position.y, camera->rcamera.position.z};
        SetShaderValue(celShader, GetShaderLocation(celShader, "viewPos"), camPos, SHADER_UNIFORM_VEC3);

        // Set light position from first directional light for cel shading
        for (int i = 0; i < MAX_LIGHTS; i++) {
          if (lightmanager.lights[i] && lightmanager.lights[i]->enabled &&
              lightmanager.lights[i]->getType() == LightType::DIRECTIONAL) {
            float lpos[3] = {lightmanager.lights[i]->position.x,
                             lightmanager.lights[i]->position.y,
                             lightmanager.lights[i]->position.z};
            SetShaderValue(celShader, GetShaderLocation(celShader, "lightPos"), lpos, SHADER_UNIFORM_VEC3);
            break;
          }
        }
      }

      // Update sea shader camera position
      if (map && map->seaShaderLoaded.id > 0 && map->seaViewPosLoc >= 0) {
        float camPos[3] = {camera->rcamera.position.x, camera->rcamera.position.y, camera->rcamera.position.z};
        SetShaderValue(map->seaShaderLoaded, map->seaViewPosLoc, camPos, SHADER_UNIFORM_VEC3);
      }

      // Shadow pass: render depth from light's perspective using CSM
      if (lightmanager.areShadowsEnabled()) {
        lightmanager.shadowFrameCounter++;
        bool shouldUpdateShadow = (lightmanager.shadowFrameCounter % lightmanager.shadowUpdateInterval) == 0;

        if (shouldUpdateShadow) {
          float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
          lightmanager.updateCascadeMatrices(camera->rcamera, nearPlane, aspect);

          lightmanager.beginShadowPass();

          Material shadowMat = lightmanager.getShadowMaterial();

          // Render all shadow casters into each cascade
          for (int c = 0; c < NUM_CASCADES; c++) {
            lightmanager.setCascade(c);

            // Map terrain
            if (map && map->model.meshCount > 0) {
              Matrix mapTransform = MatrixMultiply(map->model.transform,
                  MatrixTranslate(map->position.x, map->position.y, map->position.z));
              for (int i = 0; i < map->model.meshCount; i++) {
                DrawMesh(map->model.meshes[i], shadowMat, mapTransform);
              }
            }

            // Instanced rocks shadow pass
            auto *rocks = root.getChildOfType<EnvironmentalObject>();
            if (rocks && rocks->isVisible && rocks->getInstanceCount() > 0) {
              const auto &transforms = rocks->getTransforms();
              for (int t = 0; t < rocks->getInstanceCount(); t++) {
                DrawMesh(rocks->getMesh(), shadowMat, transforms[t]);
              }
            }

            // Characters, structures, and other shadow casters
            drawShadowCastersRecursive(&root, shadowMat);
          }

          lightmanager.endShadowPass();
        }

        // Bind shadow map texture for the main render pass (every frame)
        lightmanager.bindShadowMap();
      }

      // Push shadow uniforms AFTER the shadow pass so fragment shaders
      // use the same cascade matrices the shadow map was rendered with.
      lightmanager.updateShadowUniforms();

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
        // Brush mode: anteprima cerchio e input paint/erase
        if (inBrushMode && rocks) {
          float brushR = rocks->getBrushRadius();
          DrawCircle3D(closest.point, brushR,
                       {1.0f, 0.0f, 0.0f}, 90.0f, {0, 200, 0, 180});

          if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            rocks->paintAt(closest.point);
          }
          if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            rocks->eraseAt(closest.point);
          }
        } else {
          DrawCube(closest.point, 0.3f, 0.3f, 0.3f, ORANGE);
          Vector3 normalEnd;
          normalEnd.x = closest.point.x + closest.normal.x;
          normalEnd.y = closest.point.y + closest.normal.y;
          normalEnd.z = closest.point.z + closest.normal.z;
          DrawLine3D(closest.point, normalEnd, RED);
        }
      }

      camera->endMode3D();
      EndTextureMode();

      // Draw to screen
      camera->beginDrawing();

      // Applica l'outline shader solo se abilitato
      if (this->outlineEnabled)
      {
        BeginShaderMode(outlineShader);
      }

      DrawTextureRec(renderTarget.texture,
                     (Rectangle){0, 0, (float)renderTarget.texture.width,
                                 (float)-renderTarget.texture.height},
                     (Vector2){0, 0}, WHITE);

      if (this->outlineEnabled)
      {
        EndShaderMode();
      }

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

  void Game::drawShadowCastersRecursive(GameObject *obj, Material &shadowMat)
  {
    if (!obj) return;

    // Characters
    if (auto *character = dynamic_cast<Character *>(obj))
    {
      if (character->isVisible && character->hasModel())
      {
        Quaternion q = QuaternionFromEuler(
            character->eulerRot.x * DEG2RAD,
            character->eulerRot.y * DEG2RAD,
            character->eulerRot.z * DEG2RAD);
        Vector3 axis;
        float angle;
        QuaternionToAxisAngle(q, &axis, &angle);
        Matrix matScale = MatrixScale(character->scale, character->scale, character->scale);
        Matrix matRotation = MatrixRotate(axis, angle);
        Matrix matTranslation = MatrixTranslate(
            character->position.x, character->position.y, character->position.z);
        Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

        for (int i = 0; i < character->modelInstance.meshCount(); i++)
        {
          DrawMesh(character->modelInstance.meshes()[i], shadowMat, transform);
        }
      }
    }

    // Structures
    if (auto *structure = dynamic_cast<Structure *>(obj))
    {
      if (structure->isVisible && structure->hasModel())
      {
        Matrix matScale = MatrixScale(structure->scale, structure->scale, structure->scale);
        Matrix matRotation = QuaternionToMatrix(structure->rotation);
        Matrix matTranslation = MatrixTranslate(
            structure->position.x, structure->position.y, structure->position.z);
        Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

        for (int i = 0; i < structure->modelInstance.meshCount(); i++)
        {
          DrawMesh(structure->modelInstance.meshes()[i], shadowMat, transform);
        }
      }
    }

    // Recurse into children
    for (auto &child : obj->children)
    {
      if (child)
      {
        drawShadowCastersRecursive(child.get(), shadowMat);
      }
    }
  }

  Game::~Game()
  {
    ScriptEngine::instance().shutdown();
    lightmanager.unload();
    UnloadShader(outlineShader);
    UnloadRenderTexture(renderTarget);
    rlImGuiShutdown();
  }
} // namespace moiras
