#include "map.h"
#include "rlgl.h"
#include "../models/models.h"
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <cstdio>

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
    if (model.meshCount == 0) return;

    // Percorso file cache
    const std::string cacheFile = "../assets/navmesh.bin";

    // Prova a caricare dalla cache
    if (navMesh.loadFromFile(cacheFile)) {
        navMeshBuilt = true;
        TraceLog(LOG_INFO, "NavMesh: Loaded from cache - %d tiles, %d total polygons",
                 navMesh.getTileCount(), navMesh.getTotalPolygons());
        return;
    }

    // Cache non trovata, genera la navmesh
    BoundingBox bounds = GetMeshBoundingBox(model.meshes[0]);
    float mapWidth = bounds.max.x - bounds.min.x;
    float mapLength = bounds.max.z - bounds.min.z;
    float mapSize = fmaxf(mapWidth, mapLength);

    TraceLog(LOG_INFO, "Map size: %.1f x %.1f (max: %.1f)", mapWidth, mapLength, mapSize);

    // Configura parametri adattivi per la dimensione della mappa
    // Il sistema tiled gestisce automaticamente la suddivisione
    if (mapSize < 500.0f) {
        navMesh.m_cellSize = 0.3f;
        navMesh.m_cellHeight = 0.2f;
        navMesh.m_agentRadius = 0.6f;
        navMesh.m_agentMaxClimb = 0.9f;
        navMesh.m_agentMaxSlope = 45.0f;
        navMesh.m_minRegionArea = 8.0f;
        navMesh.m_mergeRegionArea = 20.0f;
        navMesh.m_tileSize = 32.0f;
        TraceLog(LOG_INFO, "NavMesh: Using SMALL map parameters (< 500)");
    } else if (mapSize < 1000.0f) {
        navMesh.m_cellSize = 0.4f;
        navMesh.m_cellHeight = 0.3f;
        navMesh.m_agentRadius = 0.8f;
        navMesh.m_agentMaxClimb = 1.0f;
        navMesh.m_agentMaxSlope = 45.0f;
        navMesh.m_minRegionArea = 10.0f;
        navMesh.m_mergeRegionArea = 25.0f;
        navMesh.m_tileSize = 48.0f;
        TraceLog(LOG_INFO, "NavMesh: Using SMALL-MEDIUM map parameters (500-1000)");
    } else if (mapSize < 2000.0f) {
        navMesh.m_cellSize = 0.5f;
        navMesh.m_cellHeight = 0.3f;
        navMesh.m_agentRadius = 1.0f;
        navMesh.m_agentMaxClimb = 1.0f;
        navMesh.m_agentMaxSlope = 42.0f;
        navMesh.m_minRegionArea = 12.0f;
        navMesh.m_mergeRegionArea = 30.0f;
        navMesh.m_tileSize = 64.0f;
        TraceLog(LOG_INFO, "NavMesh: Using MEDIUM map parameters (1000-2000)");
    } else if (mapSize < 4000.0f) {
        navMesh.m_cellSize = 0.8f;
        navMesh.m_cellHeight = 0.4f;
        navMesh.m_agentRadius = 1.5f;
        navMesh.m_agentMaxClimb = 1.2f;
        navMesh.m_agentMaxSlope = 40.0f;
        navMesh.m_minRegionArea = 15.0f;
        navMesh.m_mergeRegionArea = 35.0f;
        navMesh.m_tileSize = 128.0f;
        TraceLog(LOG_INFO, "NavMesh: Using LARGE map parameters (2000-4000)");
    } else {
        navMesh.m_cellSize = 1.0f;
        navMesh.m_cellHeight = 0.5f;
        navMesh.m_agentRadius = 2.0f;
        navMesh.m_agentMaxClimb = 1.5f;
        navMesh.m_agentMaxSlope = 35.0f;
        navMesh.m_minRegionArea = 20.0f;
        navMesh.m_mergeRegionArea = 50.0f;
        navMesh.m_tileSize = 256.0f;
        TraceLog(LOG_INFO, "NavMesh: Using HUGE map parameters (> 4000)");
    }

    // Costruisci la navmesh tiled
    navMeshBuilt = navMesh.buildTiled(model.meshes[0], model.transform);

    if (navMeshBuilt) {
        TraceLog(LOG_INFO, "NavMesh: Tiled build SUCCESS - %d tiles, %d total polygons",
                 navMesh.getTileCount(), navMesh.getTotalPolygons());

        // Salva nella cache
        if (navMesh.saveToFile(cacheFile)) {
            TraceLog(LOG_INFO, "NavMesh: Saved to cache file");
        }
    } else {
        TraceLog(LOG_ERROR, "NavMesh: Tiled build FAILED!");
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
        ImGui::Text("NavMesh (Tiled)");

        if (navMeshBuilt) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Ready");
            ImGui::Text("Tiles: %d", navMesh.getTileCount());
            ImGui::Text("Total Polygons: %d", navMesh.getTotalPolygons());
            ImGui::Checkbox("Show NavMesh Debug", &showNavMeshDebug);
            ImGui::Checkbox("Show Path", &showPath);
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: Not Built");

            if (ImGui::Button("Build NavMesh")) {
                buildNavMesh();
            }
        }

        ImGui::Separator();
        ImGui::Text("NavMesh Settings");
        ImGui::SliderFloat("Cell Size", &navMesh.m_cellSize, 0.1f, 5.0f);
        ImGui::SliderFloat("Cell Height", &navMesh.m_cellHeight, 0.1f, 2.0f);
        ImGui::SliderFloat("Tile Size", &navMesh.m_tileSize, 16.0f, 512.0f);
        ImGui::Separator();
        ImGui::SliderFloat("Agent Radius", &navMesh.m_agentRadius, 0.2f, 5.0f);
        ImGui::SliderFloat("Agent Height", &navMesh.m_agentHeight, 1.0f, 10.0f);
        ImGui::SliderFloat("Max Climb", &navMesh.m_agentMaxClimb, 0.1f, 5.0f);
        ImGui::SliderFloat("Max Slope", &navMesh.m_agentMaxSlope, 15.0f, 75.0f);
        ImGui::Spacing();

        if (ImGui::Button("Rebuild NavMesh (Clear Cache)")) {
            // Elimina il file cache
            const std::string cacheFile = "../assets/navmesh.bin";
            std::remove(cacheFile.c_str());
            TraceLog(LOG_INFO, "NavMesh: Cache file deleted");

            double startTime = GetTime();
            navMeshBuilt = false;
            buildNavMesh();
            double elapsed = GetTime() - startTime;
            TraceLog(LOG_INFO, "NavMesh rebuilt in %.2f seconds", elapsed);
        }
    }

    ImGui::PopID();
}

  void Map::addSea() {
    seaMesh = GenMeshPlane(5000, 5000, 50, 50);
    seaModel = LoadModelFromMesh(seaMesh);
    seaModel.materials[0].shader = seaShaderLoaded;
  }

} // namespace moiras
