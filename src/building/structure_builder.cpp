#include "structure_builder.h"
#include "../map/map.h"
#include "imgui.h"
#include <algorithm>
#include <raymath.h>

namespace moiras {

StructureBuilder::StructureBuilder()
    : GameObject("StructureBuilder"), m_buildingMode(false),
      m_selectedAsset(-1), m_lastSelectedAsset(-1), m_previewRotationY(0.0f),
      m_previewScale(1.0f), m_isValidPlacement(false), m_map(nullptr),
      m_camera(nullptr), m_navMesh(nullptr), m_modelManager(nullptr),
      m_previewModel({0}), m_previewModelLoaded(false),
      m_previewPosition({0, 0, 0}), m_previewShaderValid({0}),
      m_previewShaderInvalid({0}), m_previewShadersLoaded(false),
      m_previewTexture({0}), m_previewRenderTarget({0}), m_previewCamera({0}),
      m_lastShaderWasValid(true) {
  loadAssetList();
  loadPreviewShaders();
}

StructureBuilder::~StructureBuilder() {
  unloadPreviewModel();
  unloadPreviewShaders();
  if (m_previewTexture.id != 0) {
    UnloadTexture(m_previewTexture);
  }
  if (m_previewRenderTarget.id != 0) {
    UnloadRenderTexture(m_previewRenderTarget);
  }
}

void StructureBuilder::setMap(Map *map) { m_map = map; }

void StructureBuilder::setCamera(Camera3D *camera) { m_camera = camera; }

void StructureBuilder::setNavMesh(NavMesh *navMesh) { m_navMesh = navMesh; }

void StructureBuilder::setModelManager(ModelManager *modelManager) {
  m_modelManager = modelManager;
}

void StructureBuilder::enterBuildingMode() {
  if (m_selectedAsset >= 0 && m_selectedAsset < (int)m_assetFiles.size()) {
    m_buildingMode = true;
    loadPreviewModel("../assets/" + m_assetFiles[m_selectedAsset]);
    TraceLog(LOG_INFO, "Entered building mode with asset: %s",
             m_assetFiles[m_selectedAsset].c_str());
  }
}

void StructureBuilder::exitBuildingMode() {
  m_buildingMode = false;
  unloadPreviewModel();
  TraceLog(LOG_INFO, "Exited building mode");
}

void StructureBuilder::selectAsset(int index) {
  if (index >= 0 && index < (int)m_assetFiles.size()) {
    m_selectedAsset = index;
  }
}

void StructureBuilder::selectAsset(const std::string &assetPath) {
  for (size_t i = 0; i < m_assetFiles.size(); i++) {
    if (m_assetFiles[i] == assetPath) {
      m_selectedAsset = static_cast<int>(i);
      return;
    }
  }
}

void StructureBuilder::rotatePreview(float deltaY) {
  m_previewRotationY += deltaY;
  // Normalizza rotazione
  while (m_previewRotationY > 2 * PI)
    m_previewRotationY -= 2 * PI;
  while (m_previewRotationY < 0)
    m_previewRotationY += 2 * PI;
}

void StructureBuilder::update() {
  GameObject::update();

  // Aggiorna preview texture se cambiata la selezione
  if (m_selectedAsset != m_lastSelectedAsset && m_selectedAsset >= 0) {
    loadOrGeneratePreviewTexture(m_assetFiles[m_selectedAsset]);
    m_lastSelectedAsset = m_selectedAsset;
  }

  if (!m_buildingMode)
    return;
  if (!m_camera || !m_map)
    return;

  // Gestione input per rotazione (Q/E)
  if (IsKeyDown(KEY_Q)) {
    rotatePreview(-2.0f * GetFrameTime());
  }
  if (IsKeyDown(KEY_E)) {
    rotatePreview(2.0f * GetFrameTime());
  }

  // Scroll per scala (opzionale)
  float scroll = GetMouseWheelMove();
  if (scroll != 0.0f && IsKeyDown(KEY_LEFT_SHIFT)) {
    m_previewScale += scroll * 0.1f;
    m_previewScale = Clamp(m_previewScale, 0.1f, 10.0f);
  }

  // Aggiorna posizione preview in base al mouse
  updatePreviewPosition();

  // Verifica validita' del piazzamento
  m_isValidPlacement = checkPlacementValidity();

  // Click sinistro per piazzare
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      !ImGui::GetIO().WantCaptureMouse) {
    if (m_isValidPlacement) {
      placeStructure();
    }
  }

  // Tasto ESC o click destro per uscire dalla modalita' building
  if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    exitBuildingMode();
  }
}

void StructureBuilder::draw() {
  if (!m_buildingMode || !m_previewModelLoaded)
    return;
  if (m_previewModel.meshCount == 0)
    return;

  // Applica lo shader solo se lo stato e' cambiato
  if (m_isValidPlacement != m_lastShaderWasValid) {
    applyPreviewShader(m_isValidPlacement);
    m_lastShaderWasValid = m_isValidPlacement;
  }
  // 1. Vettore "Up" standard
  Vector3 up = {0.0f, 1.0f, 0.0f};
  Vector3 axis = Vector3CrossProduct(up, m_previewNormal);
  float dot = Vector3DotProduct(up, m_previewNormal);

  Quaternion qSurf;
  if (dot < -0.9999f)
    qSurf = QuaternionFromAxisAngle({1, 0, 0}, PI);
  else {
    qSurf = {axis.x, axis.y, axis.z, 1.0f + dot};
    qSurf = QuaternionNormalize(qSurf);
  }

  Quaternion qUser = QuaternionFromAxisAngle({0, 1, 0}, m_previewRotationY);
  Matrix matRotation = QuaternionToMatrix(QuaternionMultiply(qUser, qSurf));

  Matrix matScale = MatrixScale(m_previewScale, m_previewScale, m_previewScale);
  Matrix matTranslation = MatrixTranslate(
      m_previewPosition.x, m_previewPosition.y, m_previewPosition.z);

  m_previewModel.transform =
      MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

  DrawModel(m_previewModel, {0, 0, 0}, 1.0f, WHITE);
  // Disegna il bounding box
  BoundingBox bounds = GetModelBoundingBox(m_previewModel);
  Vector3 size = {(bounds.max.x - bounds.min.x) * m_previewScale,
                  (bounds.max.y - bounds.min.y) * m_previewScale,
                  (bounds.max.z - bounds.min.z) * m_previewScale};

  Color boxColor = m_isValidPlacement ? GREEN : RED;
  boxColor.a = 100;

  DrawCubeWires({m_previewPosition.x, m_previewPosition.y + size.y * 0.5f,
                 m_previewPosition.z},
                size.x, size.y, size.z, boxColor);

  // GameObject::draw();
}

void StructureBuilder::gui() {}

bool StructureBuilder::placeStructure() {
  if (!m_previewModelLoaded || !m_isValidPlacement)
    return false;
  if (m_selectedAsset < 0 || m_selectedAsset >= (int)m_assetFiles.size())
    return false;
  if (!m_modelManager) {
    TraceLog(LOG_ERROR, "StructureBuilder: ModelManager not set!");
    return false;
  }

  // Crea una nuova struttura
  auto structure = std::make_unique<Structure>();
  Vector3 up = {0.0f, 1.0f, 0.0f};
  Vector3 axis = Vector3CrossProduct(up, m_previewNormal);
  float dot = Vector3DotProduct(up, m_previewNormal);

  Quaternion qSurf;
  if (dot < -0.9999f)
    qSurf = QuaternionFromAxisAngle({1, 0, 0}, PI);
  else {
    qSurf = {axis.x, axis.y, axis.z, 1.0f + dot};
    qSurf = QuaternionNormalize(qSurf);
  }

  // 2. Combina con la rotazione Y dell'utente
  Quaternion qUser = QuaternionFromAxisAngle({0, 1, 0}, m_previewRotationY);
  std::string modelPath = "../assets/" + m_assetFiles[m_selectedAsset];
  structure->loadModel(*m_modelManager, modelPath);

  // Verifica che il modello sia stato caricato correttamente
  if (!structure->hasModel()) {
    TraceLog(LOG_ERROR, "Failed to load structure model: %s",
             modelPath.c_str());
    return false;
  }

  structure->position = m_previewPosition;
  structure->eulerRot = {0.0f, m_previewRotationY, 0.0f};
  structure->rotation =
      QuaternionMultiply(qUser, qSurf); // Salviamo il quaternione finale
  structure->scale = m_previewScale;
  structure->isPlaced = true;

  // Update bounds for navmesh obstacle
  structure->updateBounds();

  // Add structure as navmesh obstacle
  if (m_navMesh) {
    structure->navMeshObstacleRef = m_navMesh->addObstacle(structure->bounds);
    TraceLog(LOG_INFO, "StructureBuilder: Added navmesh obstacle ref=%u for structure",
             structure->navMeshObstacleRef);
  }

  // Imposta il nome con un contatore per renderlo unico
  static int structureCounter = 0;
  std::filesystem::path assetPath(m_assetFiles[m_selectedAsset]);
  structure->name = "Structure_" + assetPath.stem().string() + "_" +
                    std::to_string(structureCounter++);

  // Aggiungi alla scena
  GameObject *root = getRoot();
  if (root) {
    root->addChild(std::move(structure));
  }
  return true;
}

void StructureBuilder::refreshAssetList() { loadAssetList(); }

void StructureBuilder::loadAssetList() {
  m_assetFiles.clear();
  std::string assetsPath = "../assets/";

  try {
    if (std::filesystem::exists(assetsPath)) {
      for (const auto &entry :
           std::filesystem::directory_iterator(assetsPath)) {
        if (entry.is_regular_file()) {
          std::string ext = entry.path().extension().string();
          // Converti in lowercase
          for (auto &c : ext)
            c = tolower(c);

          // Filtra solo modelli (escludi la mappa principale)
          std::string filename = entry.path().filename().string();
          if ((ext == ".glb" || ext == ".obj" || ext == ".fbx" ||
               ext == ".gltf") &&
              filename != "map.glb") {
            m_assetFiles.push_back(filename);
          }
        }
      }

      // Ordina alfabeticamente
      std::sort(m_assetFiles.begin(), m_assetFiles.end());
    }
  } catch (const std::filesystem::filesystem_error &e) {
    TraceLog(LOG_WARNING, "Failed to load asset list: %s", e.what());
  }

  TraceLog(LOG_INFO, "Loaded %d building assets", (int)m_assetFiles.size());
}

void StructureBuilder::loadPreviewModel(const std::string &assetPath) {
  unloadPreviewModel();

  m_previewModel = LoadModel(assetPath.c_str());
  m_previewModelLoaded = (m_previewModel.meshCount > 0);
  if (m_previewModelLoaded) {
    // Applica lo shader immediatamente dopo il caricamento
    m_lastShaderWasValid = true; // Reset per forzare l'applicazione
    applyPreviewShader(m_isValidPlacement);
    m_lastShaderWasValid = m_isValidPlacement;
    TraceLog(LOG_INFO, "Preview model loaded: %s (meshes: %d, materials: %d)",
             assetPath.c_str(), m_previewModel.meshCount,
             m_previewModel.materialCount);
  } else {
    TraceLog(LOG_WARNING, "Failed to load preview model: %s",
             assetPath.c_str());
  }
}

void StructureBuilder::unloadPreviewModel() {
  if (m_previewModelLoaded) {
    UnloadModel(m_previewModel);
    m_previewModel = {0};
    m_previewModelLoaded = false;
  }
}

void StructureBuilder::updatePreviewPosition() {
  if (!m_camera || !m_map)
    return;

  Ray ray = GetScreenToWorldRay(GetMousePosition(), *m_camera);
  RayCollision closest = {0};
  closest.distance = FLT_MAX;

  for (int m = 0; m < m_map->model.meshCount; m++) {
    RayCollision meshHitInfo = GetRayCollisionMesh(ray, m_map->model.meshes[m],
                                                   m_map->model.transform);
    if (meshHitInfo.hit && meshHitInfo.distance < closest.distance) {
      closest = meshHitInfo;
    }
  }

  if (closest.hit) {
    m_previewPosition = closest.point;
    m_previewNormal = closest.normal; // Salviamo la normale!
  }
}
bool StructureBuilder::checkPlacementValidity() {
  if (!m_previewModelLoaded)
    return false;

  // Per ora, consideriamo valido qualsiasi piazzamento sul terreno
  // In futuro si puo' aggiungere:
  // - Controllo collisioni con altre strutture
  // - Controllo pendenza del terreno
  // - Controllo distanza minima da altre strutture

  // Verifica che la posizione sia sulla navmesh (area camminabile)
  if (m_navMesh) {
    Vector3 projectedPoint;
    bool onNavMesh =
        m_navMesh->projectPointToNavMesh(m_previewPosition, projectedPoint);

    // La struttura e' valida se il punto e' raggiungibile
    // (potrebbe essere che vogliamo piazzare strutture anche fuori dal navmesh)
    // Per ora accettiamo qualsiasi posizione sul terreno
    return true;
  }

  return true;
}

void StructureBuilder::loadPreviewShaders() {
  // Shader per preview valido (verde)
  const char *vsCode = R"(
        #version 330
        in vec3 vertexPosition;
        in vec3 vertexNormal;
        uniform mat4 mvp;
        uniform mat4 matModel;
        out vec3 fragNormal;
        void main() {
            fragNormal = normalize(mat3(matModel) * vertexNormal);
            gl_Position = mvp * vec4(vertexPosition, 1.0);
        }
    )";

  const char *fsCodeValid = R"(
        #version 330
        in vec3 fragNormal;
        out vec4 finalColor;
        void main() {
            float light = max(dot(fragNormal, normalize(vec3(1.0, 1.0, 1.0))), 0.3);
            finalColor = vec4(0.2, 0.8, 0.2, 0.7) * light;
        }
    )";

  const char *fsCodeInvalid = R"(
        #version 330
        in vec3 fragNormal;
        out vec4 finalColor;
        void main() {
            float light = max(dot(fragNormal, normalize(vec3(1.0, 1.0, 1.0))), 0.3);
            finalColor = vec4(0.8, 0.2, 0.2, 0.7) * light;
        }
    )";

  m_previewShaderValid = LoadShaderFromMemory(vsCode, fsCodeValid);
  m_previewShaderInvalid = LoadShaderFromMemory(vsCode, fsCodeInvalid);
  m_previewShadersLoaded =
      (m_previewShaderValid.id != 0 && m_previewShaderInvalid.id != 0);

  if (m_previewShadersLoaded) {
    TraceLog(LOG_INFO, "Preview shaders loaded successfully");
  } else {
    TraceLog(LOG_WARNING, "Failed to load preview shaders");
  }
}

void StructureBuilder::unloadPreviewShaders() {
  if (m_previewShaderValid.id != 0) {
    UnloadShader(m_previewShaderValid);
    m_previewShaderValid = {0};
  }
  if (m_previewShaderInvalid.id != 0) {
    UnloadShader(m_previewShaderInvalid);
    m_previewShaderInvalid = {0};
  }
  m_previewShadersLoaded = false;
}

void StructureBuilder::applyPreviewShader(bool valid) {
  if (!m_previewShadersLoaded || !m_previewModelLoaded)
    return;

  Shader shader = valid ? m_previewShaderValid : m_previewShaderInvalid;
  for (int i = 0; i < m_previewModel.materialCount; i++) {
    m_previewModel.materials[i].shader = shader;
  }
}

void StructureBuilder::setupPreviewCamera() {
  m_previewCamera.position = {3.0f, 3.0f, 3.0f};
  m_previewCamera.target = {0.0f, 0.5f, 0.0f};
  m_previewCamera.up = {0.0f, 1.0f, 0.0f};
  m_previewCamera.fovy = 45.0f;
  m_previewCamera.projection = CAMERA_PERSPECTIVE;
}

std::string StructureBuilder::getPreviewPath(const std::string &assetFilename) {
  std::filesystem::path assetPath(assetFilename);
  std::string previewName = assetPath.stem().string() + "-preview.png";
  return "../assets/" + previewName;
}

void StructureBuilder::loadOrGeneratePreviewTexture(
    const std::string &assetFilename) {
  // Scarica la texture precedente
  if (m_previewTexture.id != 0) {
    UnloadTexture(m_previewTexture);
    m_previewTexture = {0};
  }

  std::string previewPath = getPreviewPath(assetFilename);

  // Controlla se la preview esiste gia'
  if (std::filesystem::exists(previewPath)) {
    m_previewTexture = LoadTexture(previewPath.c_str());
    return;
  }

  // Genera una nuova preview
  generatePreviewTexture(assetFilename, previewPath);
}

void StructureBuilder::generatePreviewTexture(const std::string &assetFilename,
                                              const std::string &previewPath) {
  // Crea il render target se non esiste
  if (m_previewRenderTarget.id == 0) {
    m_previewRenderTarget = LoadRenderTexture(256, 256);
    setupPreviewCamera();
  }

  // Carica il modello
  std::string modelPath = "../assets/" + assetFilename;
  Model model = LoadModel(modelPath.c_str());

  if (model.meshCount == 0) {
    UnloadModel(model);
    return;
  }

  // Calcola i bounds del modello per posizionare la camera
  BoundingBox bounds = GetModelBoundingBox(model);
  Vector3 center = {(bounds.min.x + bounds.max.x) * 0.5f,
                    (bounds.min.y + bounds.max.y) * 0.5f,
                    (bounds.min.z + bounds.max.z) * 0.5f};

  float dimX = bounds.max.x - bounds.min.x;
  float dimY = bounds.max.y - bounds.min.y;
  float dimZ = bounds.max.z - bounds.min.z;
  float maxDim = std::max(std::max(dimX, dimY), dimZ);

  float distance = maxDim * 1.5f;
  m_previewCamera.position = {center.x + distance, center.y + distance * 0.5f,
                              center.z + distance};
  m_previewCamera.target = center;

  // Renderizza alla texture
  BeginTextureMode(m_previewRenderTarget);
  ClearBackground(Color{60, 60, 60, 255});
  BeginMode3D(m_previewCamera);
  DrawModel(model, {0, 0, 0}, 1.0f, WHITE);
  EndMode3D();
  EndTextureMode();

  // Salva la preview
  Image img = LoadImageFromTexture(m_previewRenderTarget.texture);
  ImageFlipVertical(&img);
  ExportImage(img, previewPath.c_str());
  UnloadImage(img);

  // Carica la preview salvata come texture
  m_previewTexture = LoadTexture(previewPath.c_str());

  // Scarica il modello
  UnloadModel(model);
}

} // namespace moiras
