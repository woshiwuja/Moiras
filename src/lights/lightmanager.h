#pragma once

#include "lights.h"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string>

namespace moiras {

constexpr int MAX_LIGHTS = 256;
constexpr int SHADOW_MAP_SIZE = 1024;
constexpr int SHADOW_TEXTURE_SLOT = 14;

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

  // Shadow mapping
  void setupShadowMap(const std::string &depthVsPath, const std::string &depthFsPath);
  void registerShadowShader(Shader targetShader); // Call once during setup
  void updateLightSpaceMatrix(Vector3 cameraPos);
  void updateShadowUniforms(); // Call each frame to push matrix/bias to registered shaders
  void beginShadowPass();
  void endShadowPass();
  void bindShadowMap();
  Shader getShadowDepthShader() const { return shadowDepthShader; }
  Material getShadowMaterial() const { return shadowMaterial; }
  Matrix getLightSpaceMatrix() const { return lightSpaceMatrix; }
  bool areShadowsEnabled() const { return shadowsEnabled && shadowMapReady; }
  int shadowUpdateInterval = 2; // Update shadow map every N frames

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
  bool useTiling = false;

  // Shadow settings (public for GUI and game loop)
  bool shadowsEnabled = true;
  float shadowOrthoSize = 150.0f;
  float shadowNear = 1.0f;
  float shadowFar = 500.0f;
  float shadowBias = 0.005f;
  int shadowFrameCounter = 0;

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

  // Shadow map resources
  Shader shadowDepthShader = {0};
  Material shadowMaterial = {0};
  unsigned int shadowMapFBO = 0;
  unsigned int shadowMapDepthTex = 0;
  Matrix lightSpaceMatrix = {0};
  bool shadowMapReady = false;

  // Cached shadow uniform locations per shader
  struct ShadowShaderLocs {
    Shader shader = {0};
    int shadowEnabledLoc = -1;
    int lightSpaceMatrixLoc = -1;
    int shadowMapLoc = -1;
    int shadowBiasLoc = -1;
  };
  static constexpr int MAX_SHADOW_SHADERS = 4;
  ShadowShaderLocs shadowShaders[MAX_SHADOW_SHADERS];
  int shadowShaderCount = 0;

  // Saved state for shadow pass
  Matrix savedProjection = {0};
  Matrix savedModelview = {0};

  void updatePBRUniforms();
};

} // namespace moiras
