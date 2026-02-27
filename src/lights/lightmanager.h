#pragma once

#include "lights.h"
#include <raylib.h>
#include <r3d/r3d.h>
#include <string>

namespace moiras {

constexpr int MAX_LIGHTS = 256;

// LightManager is now a thin wrapper that registers scene-graph Light nodes
// for GUI display and delegates all actual lighting to r3d.
class LightManager {
public:
    LightManager();
    ~LightManager() = default;

    // Register a scene-graph light (for GUI display only)
    int addLight(Light *light);
    void removeLight(int index);

    // GUI for lighting settings (controls r3d environment)
    void gui();

    // Stub methods retained for API compatibility
    void unload() {}
    void updateAllLights() {}
    void updateCameraPosition(Vector3) {}
    void loadShader(const std::string &, const std::string &) {}
    void setupShadowMap(const std::string &, const std::string &) {}
    void registerShadowShader(Shader) {}
    bool areShadowsEnabled() const { return false; }
    Shader getShader() const { return {0}; }

    // Light registry (for GUI display)
    Light *lights[MAX_LIGHTS] = {nullptr};
    int lightCount = 0;
};

} // namespace moiras
