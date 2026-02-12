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
#include "../audio/audiodevice.hpp"
#include "../scripting/ScriptEngine.hpp"
#include "../scripting/ScriptComponent.hpp"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <cstdio>
#include <memory>
#include <raymath.h>
#include <iostream>

namespace moiras
{

Game::Game() { 
    auto root = std::make_unique<GameObject>(); 
}

Game::~Game() {
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void Game::renderLoadingFrame(const char *message, float progress) {
    // TODO: Implement loading screen with Filament
    // For now, just print to console
    std::cout << "[Loading " << (int)(progress * 100) << "%] " << message << std::endl;
}

void Game::setup()
{
    // Initialize SDL window (800x600 for now, will be resizable)
    m_window.init(1920, 1080, "Moiras - Filament Migration", false);
    
    // Initialize Filament engine
    m_filamentEngine.init(m_window.getNativeWindow(), 
                          m_window.getWidth(), 
                          m_window.getHeight());

    // Initialize ImGui with SDL2 + OpenGL3 backends
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui_ImplSDL2_InitForOpenGL(m_window.getSDLWindow(), m_window.getGLContext());
    ImGui_ImplOpenGL3_Init("#version 450");
    
    std::cout << "Filament + SDL initialized successfully" << std::endl;

    renderLoadingFrame("Inizializzazione scripting...", 0.0f);

    // Initialize the Lua scripting engine
    ScriptEngine::instance().initialize();
    ScriptEngine::instance().setGameRoot(&root);
    ScriptEngine::instance().setGame(this);
    ScriptEngine::instance().setScriptsDirectory("../assets/scripts");

    // Create main camera
    auto mainCamera = std::make_unique<GameCamera>("MainCamera");

    renderLoadingFrame("Caricamento mappa...", 0.10f);

    // TODO Phase 2: Load map via gltfio instead of Raylib
    // For Phase 1, skip map loading to get basic window working
    // std::unique_ptr<Map> map = moiras::mapFromModel("../assets/map.glb");

    renderLoadingFrame("Inizializzazione interfaccia...", 0.20f);

    auto gui = std::make_unique<Gui>();
    gui->setModelManager(&modelManager);
    auto sidebar = gui->getChildOfType<Sidebar>();
    if (sidebar)
    {
        sidebar->lightManager = &lightmanager;
        sidebar->modelManager = &modelManager;
        sidebar->outlineEnabled = &this->outlineEnabled;
        std::cout << "LightManager and ModelManager linked to Sidebar" << std::endl;
    }

    // Create script editor as child of GUI
    auto scriptEditorPtr = std::make_unique<ScriptEditor>();
    scriptEditor = scriptEditorPtr.get();
    scriptEditor->setOpen(false); // Start closed, can be toggled with F12
    gui->addChild(std::move(scriptEditorPtr));

    // Link script editor to sidebar
    if (sidebar)
    {
        sidebar->scriptEditor = scriptEditor;
    }

    std::cout << "Script Editor initialized (press F12 to open)" << std::endl;
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
        std::cout << "StructureBuilder linked to Sidebar" << std::endl;
    }

    renderLoadingFrame("Inizializzazione rendering...", 0.40f);

    nearPlane = 0.5f;
    farPlane = 50000.0f;

    // TODO Phase 2: Load PBR material for terrain
    // TODO Phase 3: Initialize lights and shadows
    // For Phase 1, we'll use default Filament rendering

    lightmanager = LightManager();
    
    // TODO Phase 3: Migrate lights to Filament
    // auto light1 = std::make_unique<DirectionalLight>("Light1");
    // light1->position = {100.0f, 100.0f, 100.0f};
    // ...

    root.addChild(std::move(mainCamera));
    // root.addChild(std::move(map));  // TODO Phase 2
    root.addChild(std::move(gui));
    // root.addChild(std::move(lights));  // TODO Phase 3

    // TODO Phase 2: Build NavMesh
    renderLoadingFrame("Setup completato!", 1.0f);

    std::cout << "\n=== Phase 1 Setup Complete ===" << std::endl;
    std::cout << "Window: SDL2 with OpenGL 4.5" << std::endl;
    std::cout << "Renderer: Filament" << std::endl;
    std::cout << "ImGui: SDL2 + OpenGL3 backends" << std::endl;
    std::cout << "Ready to render!" << std::endl;
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

void Game::loop()
{
    while (!m_window.shouldClose())
    {
        // ===== EVENT POLLING =====
        m_window.pollEvents();

        // ===== UPDATE PHASE =====
        // Update TimeManager first (caches delta time for the frame)
        TimeManager::getInstance().update();

        // Update input manager and set context based on game state
        InputManager &input = InputManager::getInstance();

        // Determine current context
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

        // Update input state (must be called before any input queries)
        input.update();

        // Handle pause toggle (works in any context)
        if (input.isActionJustPressed(InputAction::UI_TOGGLE_PAUSE))
        {
            TimeManager::getInstance().togglePause();
        }

        // Handle speed changes (works in any context)
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

        // Toggle script editor with F12 (always works)
        if (input.isActionJustPressed(InputAction::UI_TOGGLE_SCRIPT_EDITOR) && scriptEditor)
        {
            scriptEditor->setOpen(!scriptEditor->isOpen());
        }

        // Update GameObject hierarchy
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
        // auto map = root.getChildOfType<Map>();  // TODO Phase 2

        // Update player controller (only if not in building mode or brush mode)
        // auto *rocks = root.getChildOfType<EnvironmentalObject>();
        bool inBuildingMode = (structureBuilder && structureBuilder->isBuildingMode());
        // bool inBrushMode = (rocks && rocks->isBrushMode());
        if (playerController && camera && !inBuildingMode)
        {
            playerController->update(camera);
        }

        // TODO Phase 3: Update lights
        // lightmanager.updateCameraPosition(camera->position);
        // lightmanager.updateAllLights();

        // ===== RENDER PHASE =====
        if (m_filamentEngine.beginFrame())
        {
            // Render scene (currently empty, will add entities in Phase 2)
            m_filamentEngine.render();

            // ===== IMGUI OVERLAY =====
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // Render GameObject GUI hierarchy
            root.gui();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            m_filamentEngine.endFrame();
        }

        // Swap buffers
        m_window.swapBuffers();
    }
}

} // namespace moiras
