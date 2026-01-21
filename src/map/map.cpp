#include "map.h"
#include "../models/models.h"
#include <raylib.h>
#include <raymath.h>

namespace moiras {

Map::Map() : width(0), height(0), length(0), model{}, mesh{}, texture{} {}

Map::Map(float width_, float height_, float length_, Model model_, Mesh mesh_,
         Texture texture_)
    : width(width_), height(height_), length(length_), model(model_),
      mesh(mesh_), texture(texture_) {}

Map::~Map() {
  if (model.meshCount > 0) {
    UnloadModel(model);
  }
  if (mesh.vertexCount > 0) {
    UnloadMesh(mesh);
  }
  if (texture.id > 0) {
    UnloadTexture(texture);
  }
  UnloadShader(seaShaderLoaded);
  UnloadModel(seaModel);
  UnloadTexture(perlinNoiseMap);
}

Map::Map(Map &&other) noexcept
    : GameObject(std::move(other)), width(other.width), height(other.height),
      length(other.length), model(other.model), mesh(other.mesh),
      texture(other.texture) {
  other.model = {};
  other.mesh = {};
  other.texture = {};
}

Map::Map(Model model_) : model(model_) {}
Map &Map::operator=(Map &&other) noexcept {
  if (this != &other) {
    if (model.meshCount > 0)
      UnloadModel(model);
    if (mesh.vertexCount > 0)
      UnloadMesh(mesh);
    if (texture.id > 0)
      UnloadTexture(texture);

    width = other.width;
    height = other.height;
    length = other.length;
    model = other.model;
    mesh = other.mesh;
    texture = other.texture;

    other.model = {};
    other.mesh = {};
    other.texture = {};
  }
  return *this;
}

void Map::draw() {
  Vector3 offset = {0,3,0};
  DrawModel(model, position, 2.0f, WHITE);
  BeginShaderMode(seaShaderLoaded);
  DrawModel(seaModel, Vector3Add(position, offset), 1.0f, WHITE);
  EndShaderMode();
}

std::unique_ptr<Map> mapFromHeightmap(const std::string &filename, float width,
                                      float height, float length) {
  Image image = LoadImage(filename.c_str());
  Texture texture = LoadTextureFromImage(image);
  Mesh mesh = GenMeshHeightmap(image, Vector3{width, height, length});
  auto exported = ExportMesh(mesh, "map.obj");
  smoothMesh(&mesh, 7, .8);
  GenMeshTangents(&mesh);
  Model model = LoadModelFromMesh(mesh);

  model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
  UnloadImage(image);
  return std::make_unique<Map>(width, height, length, model, mesh, texture);
}

std::unique_ptr<Map> mapFromModel(const std::string &filename) {
  auto model = LoadModel(filename.c_str());
  // Calcola il bounding box del modello
  BoundingBox bounds = GetModelBoundingBox(model);
  
  // Calcola il centro
  Vector3 center = {
    (bounds.min.x + bounds.max.x) / 2.0f,
    (bounds.min.y + bounds.max.y) / 2.0f,
    (bounds.min.z + bounds.max.z) / 2.0f
  };
  
  // Centra il modello traslando di -center
  Matrix translation = MatrixTranslate(-center.x, -center.y, -center.z);
  model.transform = MatrixMultiply(model.transform, translation);
  return std::make_unique<Map>(model);
}
void Map::loadSeaShader() {
  seaShaderLoaded =
      LoadShader(seaShaderVertex.c_str(), seaShaderFragment.c_str());
  perlinNoiseImage = GenImagePerlinNoise(5000, 5000, 0, 0, 1.0f);
  perlinNoiseMap = LoadTextureFromImage(perlinNoiseImage);
  UnloadImage(perlinNoiseImage);
  int perlinNoiseMapLoc = GetShaderLocation(seaShaderLoaded, "perlinNoiseMap");
  rlEnableShader(seaShaderLoaded.id);
  rlActiveTextureSlot(1);
  rlEnableTexture(perlinNoiseMap.id);
  rlSetUniformSampler(perlinNoiseMapLoc, 1);
}

void Map::update() {
  hiddenTimeCounter += GetFrameTime();
  SetShaderValue(seaShaderLoaded, GetShaderLocation(seaShaderLoaded, "time"),
                 &hiddenTimeCounter, SHADER_UNIFORM_FLOAT);
};
void Map::loadSkybox(const std::string& texturePath) {
    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    skyboxModel = LoadModelFromMesh(cube);
    skyboxTexture = LoadTexture(texturePath.c_str());
    skyboxShader = LoadShader(skyboxShaderVertex.c_str(), 
                              skyboxShaderFragment.c_str());
    skyboxModel.materials[0].shader = skyboxShader;
    skyboxModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = skyboxTexture;
}

void Map::drawSkybox(Vector3 cameraPosition) {
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    BeginShaderMode(skyboxShader);
    DrawModel(skyboxModel, cameraPosition, 1.0f, WHITE);
    EndShaderMode();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}
} // namespace moiras
