#pragma once

#include "lights.h"
#include <raylib.h>
#include <string>

namespace moiras {

constexpr int MAX_LIGHTS = 4;

class LightManager {
public:
  LightManager();
  ~LightManager() = default;

  void loadShader(const std::string &vsPath, const std::string &fsPath);
  int addLight(Light *light);
  void removeLight(int index);
  void updateAllLights();
  void updateCameraPosition(Vector3 cameraPos);
  Shader getShader() const { return shader; }
  void setAmbient(Color color, float intensity);
  void unload();

  void applyMaterial(const Material &material);
  // GUI per ImGui
  void gui();

  // PBR parameters (pubblici per accesso da GUI)
  float metallicValue = 0.0f;
  float roughnessValue = 0.5f;
  float aoValue = 1.0f;
  float normalValue = 1.0f;
  float emissivePower = 0.0f;
  float ambientIntensity = 0.3f;
  Color ambientColor = {128, 128, 128, 255};
  float albedoColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float emissiveColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float tiling[2] = {1.0f, 1.0f};
  float offset[2] = {0.0f, 0.0f};

  // Texture toggles
  bool useTexAlbedo = true;
  bool useTexNormal = false;
  bool useTexMRA = false;
  bool useTexEmissive = false;

  // Accesso alle luci
  Light *lights[MAX_LIGHTS] = {nullptr};
  int lightCount = 0;
  int useTilingLoc = -1;
  bool useTiling = false; // Default: no tiling
private:
  Shader shader = {0};

  int viewPosLoc = -1;
  int ambientColorLoc = -1;
  int ambientIntensityLoc = -1;

  // PBR uniform locations
  int metallicLoc = -1;
  int roughnessLoc = -1;
  int aoLoc = -1;
  int normalLoc = -1;
  int emissivePowerLoc = -1;
  int albedoColorLoc = -1;
  int emissiveColorLoc = -1;
  int tilingLoc = -1;
  int offsetLoc = -1;
  int useTexAlbedoLoc = -1;
  int useTexNormalLoc = -1;
  int useTexMRALoc = -1;
  int useTexEmissiveLoc = -1;

  void updatePBRUniforms();
};

} // namespace moiras
