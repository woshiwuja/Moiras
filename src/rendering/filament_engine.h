#pragma once

#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Camera.h>
#include <filament/SwapChain.h>
#include <filament/Skybox.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <utils/Entity.h>
#include <utils/EntityManager.h>

namespace moiras {

class FilamentEngine {
private:
    filament::Engine* m_engine = nullptr;
    filament::Renderer* m_renderer = nullptr;
    filament::Scene* m_scene = nullptr;
    filament::View* m_view = nullptr;
    filament::SwapChain* m_swapChain = nullptr;
    filament::Camera* m_camera = nullptr;
    utils::Entity m_cameraEntity;

    // Frame tracking
    bool m_frameStarted = false;

public:
    FilamentEngine();
    ~FilamentEngine();

    // Initialize Filament with native window handle
    void init(void* nativeWindow, int width, int height);

    // Resize viewport
    void resize(int width, int height);

    // Accessors
    filament::Engine* getEngine() { return m_engine; }
    filament::Scene* getScene() { return m_scene; }
    filament::View* getView() { return m_view; }
    filament::Renderer* getRenderer() { return m_renderer; }
    filament::Camera* getCamera() { return m_camera; }
    utils::Entity getCameraEntity() { return m_cameraEntity; }

    // Frame management
    bool beginFrame();
    void render();
    void endFrame();

    // Utility methods
    filament::Material* loadMaterial(const std::string& path);
};

} // namespace moiras
