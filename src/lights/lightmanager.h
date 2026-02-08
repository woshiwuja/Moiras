#pragma once

#include "lights.h"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string>

namespace moiras {

constexpr int MAX_LIGHTS = 256;
constexpr int NUM_CASCADES = 4;
constexpr int CASCADE_SIZE = 2048;
constexpr int SHADOW_ATLAS_SIZE = CASCADE_SIZE * 2; // 4096 = 2x2 grid
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

  // Shadow mapping (CSM)
  void setupShadowMap(const std::string &depthVsPath, const std::string &depthFsPath);
  void registerShadowShader(Shader targetShader);
  void updateCascadeMatrices(const Camera3D &camera, float cameraNear, float screenAspect);
  void updateShadowUniforms();
  void beginShadowPass();
  void setCascade(int cascade);
  void endShadowPass();
  void bindShadowMap();
  Shader getShadowDepthShader() const { return shadowDepthShader; }
  Material getShadowMaterial() const { return shadowMaterial; }
  bool areShadowsEnabled() const { return shadowsEnabled && shadowMapReady; }
  int shadowUpdateInterval = 1;

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
  bool shadowsEnabled = false;
  float shadowFar = 500.0f;
  float shadowBias = 0.002f;
  float shadowNormalOffset = 0.3f;
  float cascadeLambda = 0.5f; // 0=uniform splits, 1=logarithmic splits
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

  // Shadow map resources (CSM)
  Shader shadowDepthShader = {0};
  Material shadowMaterial = {0};
  unsigned int shadowMapFBO = 0;
  unsigned int shadowMapDepthTex = 0;
  Matrix cascadeMatrices[NUM_CASCADES] = {};
  float cascadeSplits[NUM_CASCADES] = {};
  bool shadowMapReady = false;

  // Cached shadow uniform locations per shader
  struct ShadowShaderLocs {
    Shader shader = {0};
    int shadowEnabledLoc = -1;
    int cascadeMatricesLoc[NUM_CASCADES] = {-1, -1, -1, -1};
    int cascadeSplitsLoc = -1;
    int shadowMapLoc = -1;
    int shadowBiasLoc = -1;
    int shadowNormalOffsetLoc = -1;
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
