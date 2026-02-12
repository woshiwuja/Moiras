#include "filament_engine.h"
#include <fstream>
#include <vector>
#include <stdexcept>
#include <iostream>

using namespace filament;
using namespace utils;

namespace moiras {

FilamentEngine::FilamentEngine()
    : m_engine(nullptr)
    , m_renderer(nullptr)
    , m_scene(nullptr)
    , m_view(nullptr)
    , m_swapChain(nullptr)
    , m_camera(nullptr)
    , m_frameStarted(false)
{
}

FilamentEngine::~FilamentEngine() {
    if (m_engine) {
        if (m_camera) {
            m_engine->destroyCameraComponent(m_cameraEntity);
        }
        if (m_swapChain) {
            m_engine->destroy(m_swapChain);
        }
        if (m_view) {
            m_engine->destroy(m_view);
        }
        if (m_scene) {
            m_engine->destroy(m_scene);
        }
        if (m_renderer) {
            m_engine->destroy(m_renderer);
        }

        Engine::destroy(&m_engine);
    }
}

void FilamentEngine::init(void* nativeWindow, int width, int height) {
    // Create Filament engine with OpenGL backend
    m_engine = Engine::create(Engine::Backend::OPENGL);
    if (!m_engine) {
        throw std::runtime_error("Failed to create Filament engine");
    }

    std::cout << "Filament Engine created" << std::endl;

    // Create SwapChain from native window
    m_swapChain = m_engine->createSwapChain(nativeWindow);
    if (!m_swapChain) {
        throw std::runtime_error("Failed to create Filament SwapChain");
    }

    // Create Renderer
    m_renderer = m_engine->createRenderer();
    if (!m_renderer) {
        throw std::runtime_error("Failed to create Filament Renderer");
    }

    // Configure renderer clear options
    m_renderer->setClearOptions({
        .clearColor = {0.1f, 0.125f, 0.25f, 1.0f},  // DARKBLUE similar to Raylib
        .clear = true
    });

    // Create Scene
    m_scene = m_engine->createScene();
    if (!m_scene) {
        throw std::runtime_error("Failed to create Filament Scene");
    }

    // Create View
    m_view = m_engine->createView();
    if (!m_view) {
        throw std::runtime_error("Failed to create Filament View");
    }

    m_view->setScene(m_scene);
    m_view->setViewport({0, 0, (uint32_t)width, (uint32_t)height});

    // Enable shadows (for Phase 3)
    m_view->setShadowingEnabled(true);
    m_view->setShadowType(View::ShadowType::PCF);  // Percentage Closer Filtering

    // Disable default post-processing effects for now
    m_view->setPostProcessingEnabled(false);

    // Create Camera entity and component
    m_cameraEntity = EntityManager::get().create();
    m_camera = m_engine->createCamera(m_cameraEntity);

    // Set default camera projection (match Raylib's settings)
    // FOV 45°, aspect 16:9, near 0.5, far 50000
    float aspect = (float)width / (float)height;
    m_camera->setProjection(45.0, aspect, 0.5, 50000.0, Camera::Fov::VERTICAL);

    // Set default camera position (looking down at origin)
    math::mat4f viewMatrix = math::mat4f::lookAt(
        math::float3(0, 10, 20),   // Eye position
        math::float3(0, 0, 0),     // Look at origin
        math::float3(0, 1, 0)      // Up vector
    );
    m_camera->setModelMatrix(inverse(viewMatrix));

    // Attach camera to view
    m_view->setCamera(m_camera);

    std::cout << "Filament initialized: " << width << "x" << height << std::endl;
}

void FilamentEngine::resize(int width, int height) {
    if (m_view) {
        m_view->setViewport({0, 0, (uint32_t)width, (uint32_t)height});
    }

    if (m_camera) {
        float aspect = (float)width / (float)height;
        m_camera->setProjection(45.0, aspect, 0.5, 50000.0, Camera::Fov::VERTICAL);
    }
}

bool FilamentEngine::beginFrame() {
    if (!m_renderer || !m_swapChain) {
        return false;
    }

    // Begin frame
    if (m_renderer->beginFrame(m_swapChain)) {
        m_frameStarted = true;
        return true;
    }

    return false;
}

void FilamentEngine::render() {
    if (!m_frameStarted || !m_renderer || !m_view) {
        return;
    }

    // Render the view
    m_renderer->render(m_view);
}

void FilamentEngine::endFrame() {
    if (!m_frameStarted || !m_renderer) {
        return;
    }

    m_renderer->endFrame();
    m_frameStarted = false;
}

filament::Material* FilamentEngine::loadMaterial(const std::string& path) {
    // Read compiled material file (.filamat)
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open material file: " << path << std::endl;
        return nullptr;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "Failed to read material file: " << path << std::endl;
        return nullptr;
    }

    // Create material from package
    Material* material = Material::Builder()
        .package(buffer.data(), buffer.size())
        .build(*m_engine);

    if (!material) {
        std::cerr << "Failed to build material from: " << path << std::endl;
        return nullptr;
    }

    std::cout << "Loaded material: " << path << std::endl;
    return material;
}

} // namespace moiras
