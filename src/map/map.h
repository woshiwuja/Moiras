#pragma once
#include "../game/game_object.h"
#include "../navigation/navmesh.h"
#include "rlgl.h"
#include <raylib.h>
#include <functional>
#include <string>
namespace moiras {
class Map : public GameObject {
public:
  void buildNavMesh(NavMesh::ProgressCallback progressCallback = nullptr);
  void drawNavMeshDebug();

NavMesh navMesh;
bool navMeshBuilt = false;
bool showNavMeshDebug = false;

// Pathfinding debug
bool showPath = true;
Vector3 pathStart = {0, 0, 0};
Vector3 pathEnd = {10, 0, 10};
std::vector<Vector3> debugPath;
  Shader seaShaderLoaded;
  float hiddenTimeCounter = 0;
  Image perlinNoiseImage;
  Texture perlinNoiseMap;

  // Sea shader uniform locations
  int seaTimeLoc = -1;
  int seaViewPosLoc = -1;
  int seaLightDirLoc = -1;
  int seaDeepColorLoc = -1;
  int seaShallowColorLoc = -1;
  int seaFoamThresholdLoc = -1;

  // Sea shader tunable parameters
  float seaLightDir[3] = {0.5f, 0.8f, 0.3f};
  float seaDeepColor[4] = {0.0f, 0.08f, 0.18f, 0.9f};
  float seaShallowColor[4] = {0.1f, 0.4f, 0.5f, 0.8f};
  float seaFoamThreshold = 0.65f;
  float width;
  float height;
  float length;
  Vector3 position = {0., 0., 0.};
  Model model;
  Mesh mesh;
  Texture texture;
  std::string seaShaderVertex;
  std::string seaShaderFragment;
  Mesh seaMesh;
  Model seaModel;
  Model skyboxModel;
  Shader skyboxShader;
  Texture skyboxTexture;
  std::string skyboxShaderVertex;
  std::string skyboxShaderFragment;
  Map();
  Map(float width_, float height_, float length_, Model model_, Mesh mesh_,
      Texture texture_);
  Map(Model model_);
  Map(const Map &) = delete;
  Map &operator=(const Map &) = delete;
  Map(Map &&other) noexcept;
  Map &operator=(Map &&other) noexcept;
  ~Map();
  void draw() override;
  void loadSeaShader();
  void setFog();
  void addSea();
  void loadSkybox(const std::string &texturePath);
  void drawSkybox(Vector3 cameraPosition);
  void update() override;
  void gui() override;
};
std::unique_ptr<Map> mapFromHeightmap(const std::string &filename, float width,
                                      float height, float lenght);
std::unique_ptr<Map> mapFromModel(const std::string &filename);

} // namespace moiras
