#include "map.h"
#include "rlgl.h"
#include "../models/models.h"
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>

namespace moiras {

Map::Map() : width(0), height(0), length(0), model{}, mesh{}, texture{} {
  name = "Map";
}

Map::Map(float width_, float height_, float length_, Model model_, Mesh mesh_,
         Texture texture_)
    : width(width_), height(height_), length(length_), model(model_),
      mesh(mesh_), texture(texture_) {
  name = "Map";
}

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
  name = "Map";
  other.model = {};
  other.mesh = {};
  other.texture = {};
}

Map::Map(Model model_) : model(model_) { name = "Map"; }
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
    name = other.name;
  }
  return *this;
}

void Map::draw() {
  DrawModel(model, position, 1.0f, WHITE);
  BeginShaderMode(seaShaderLoaded);
  DrawModel(seaModel, position, 1.0f, WHITE);
  EndShaderMode();
  if (showNavMeshDebug && navMeshBuilt) {
    navMesh.drawDebug();
  };
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
  Vector3 center = {(bounds.min.x + bounds.max.x) / 2.0f,
                    (bounds.min.y + bounds.max.y) / 2.0f,
                    (bounds.min.z + bounds.max.z) / 2.0f};

  // Centra il modello traslando di -center
  Matrix translation = MatrixTranslate(-center.x, -center.y, -center.z);
  model.transform = MatrixMultiply(model.transform, translation);
  return std::make_unique<Map>(model);
}
void Map::loadSeaShader() {
  seaShaderLoaded =
      LoadShader(seaShaderVertex.c_str(), seaShaderFragment.c_str());
  perlinNoiseImage = GenImagePerlinNoise(500, 500, 1, 1, 1.0f);
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

void Map::buildNavMesh() {
    if (model.meshCount > 0) {
        BoundingBox bounds = GetMeshBoundingBox(model.meshes[0]);
        float mapWidth = bounds.max.x - bounds.min.x;
        float mapLength = bounds.max.z - bounds.min.z;
        float mapSize = fmaxf(mapWidth, mapLength);

        TraceLog(LOG_INFO, "Map size: %.1f x %.1f (max: %.1f)", mapWidth, mapLength, mapSize);

        // Calcolo intelligente del cellSize per mappe grandi
        // Obiettivo: grid tra 1000 e 8000 celle per lato
        float targetGridSize = 5000.0f;  // Era 500, ora 5000 per mappe grandi
        float suggestedCellSize = mapSize / targetGridSize;

        // Clamp tra 0.3 e 2.0 per evitare estremi
        navMesh.m_cellSize = fmaxf(0.3f, fminf(2.0f, suggestedCellSize));
        navMesh.m_cellHeight = fmaxf(0.3f, navMesh.m_cellSize * 0.6f);

        // Per mappe grandi, usa parametri ottimizzati
        if (mapSize >= 2000.0f) {
            navMesh.m_agentRadius = 0.8f;
            navMesh.m_agentMaxClimb = 0.9f;
            navMesh.m_agentMaxSlope = 45.0f;
        }

        TraceLog(LOG_INFO, "Using cellSize: %.2f, cellHeight: %.2f (grid ~%.0f x %.0f)",
                 navMesh.m_cellSize, navMesh.m_cellHeight,
                 mapWidth / navMesh.m_cellSize, mapLength / navMesh.m_cellSize);

        // Passa la trasformazione del modello!
        navMeshBuilt = navMesh.build(model.meshes[0], model.transform);
    }
}

void Map::drawNavMeshDebug() {
  if (navMeshBuilt) {
    navMesh.drawDebug();
  }
}
void Map::gui() {
    ImGui::PushID(this);
    
    if (ImGui::CollapsingHeader("Map")) {
        ImGui::Text("Meshes: %d", model.meshCount);
        ImGui::Text("Materials: %d", model.materialCount);
        
        ImGui::Separator();
        ImGui::Text("NavMesh");
        
        if (navMeshBuilt) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Ready");
            ImGui::Checkbox("Show Debug", &showNavMeshDebug);
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: Not Built");
            
            if (ImGui::Button("Build NavMesh")) {
                buildNavMesh();
            }
        }
        
        ImGui::Separator();
        ImGui::Text("NavMesh Settings");
        ImGui::SliderFloat("Cell Size", &navMesh.m_cellSize, 1.0f, 20.0f);
        ImGui::SliderFloat("Cell Height", &navMesh.m_cellHeight, 0.5f, 10.0f);
        ImGui::SliderFloat("Agent Radius", &navMesh.m_agentRadius, 0.5f, 5.0f);
        ImGui::SliderFloat("Agent Height", &navMesh.m_agentHeight, 1.0f, 10.0f);
        ImGui::SliderFloat("Max Climb", &navMesh.m_agentMaxClimb, 0.5f, 5.0f);
        ImGui::SliderFloat("Max Slope", &navMesh.m_agentMaxSlope, 15.0f, 75.0f);
ImGui::Spacing();
if (ImGui::Button("Rebuild NavMesh")) {
    double startTime = GetTime();
    navMeshBuilt = false;
    buildNavMesh();
    double elapsed = GetTime() - startTime;
    TraceLog(LOG_INFO, "NavMesh rebuilt in %.2f seconds", elapsed);
}
    }
    
    ImGui::PopID();
};

  void Map::addSea() {
    seaMesh = GenMeshPlane(5000, 5000, 50, 50);
    seaModel = LoadModelFromMesh(seaMesh);
    seaModel.materials[0].shader = seaShaderLoaded;
  }

} // namespace moiras
