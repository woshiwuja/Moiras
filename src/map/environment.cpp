#include "environment.hpp"
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <filesystem>

namespace moiras {

const char *rockMeshTypeName(RockMeshType type)
{
    switch (type) {
        case RockMeshType::CUBE:       return "Cube";
        case RockMeshType::SPHERE:     return "Sphere";
        case RockMeshType::HEMISPHERE: return "Hemisphere";
        case RockMeshType::CYLINDER:   return "Cylinder";
        case RockMeshType::CONE:       return "Cone";
        case RockMeshType::CUSTOM:     return "Custom";
        default:                       return "Unknown";
    }
}

const char *patchDisplayName(const RockPatch &patch)
{
    if (patch.meshType == RockMeshType::CUSTOM && !patch.customName.empty()) {
        return patch.customName.c_str();
    }
    return rockMeshTypeName(patch.meshType);
}

EnvironmentalObject::EnvironmentalObject(float rockSize, float spawnRadius)
    : GameObject("Rocks"),
      m_instancingShader{0},
      m_terrain(nullptr),
      m_rockSize(rockSize),
      m_spawnRadius(spawnRadius),
      m_initialized(false),
      m_shaderLoaded(false),
      m_cameraPos{0},
      m_cullDistance(150.0f),
      m_brushMode(false),
      m_brushRadius(10.0f),
      m_brushDensity(5),
      m_activePatch(0)
{
    scanModelFiles();
}

EnvironmentalObject::~EnvironmentalObject()
{
    for (auto &patch : m_patches) {
        UnloadMesh(patch.mesh);
        UnloadMaterial(patch.material);
    }
    if (m_shaderLoaded) {
        UnloadShader(m_instancingShader);
    }
}

Mesh EnvironmentalObject::generateMesh(RockMeshType type, float size)
{
    switch (type) {
        case RockMeshType::SPHERE:
            return GenMeshSphere(size * 0.5f, 8, 8);
        case RockMeshType::HEMISPHERE:
            return GenMeshHemiSphere(size * 0.5f, 8, 8);
        case RockMeshType::CYLINDER:
            return GenMeshCylinder(size * 0.4f, size * 0.7f, 8);
        case RockMeshType::CONE:
            return GenMeshCone(size * 0.5f, size * 0.8f, 8);
        case RockMeshType::CUBE:
        default:
            return GenMeshCube(size, size * 0.7f, size);
    }
}

void EnvironmentalObject::loadShader()
{
    if (m_shaderLoaded) return;

    m_instancingShader = LoadShader("../assets/shaders/instancing.vs",
                                    "../assets/shaders/instancing.fs");
    m_instancingShader.locs[SHADER_LOC_MATRIX_MVP] =
        GetShaderLocation(m_instancingShader, "mvp");
    m_instancingShader.locs[SHADER_LOC_MATRIX_MODEL] =
        GetShaderLocationAttrib(m_instancingShader, "instanceTransform");
    m_shaderLoaded = true;
}

void EnvironmentalObject::scanModelFiles()
{
    m_modelFiles.clear();
    std::string assetsPath = "../assets/";

    try {
        if (!std::filesystem::exists(assetsPath)) return;

        for (const auto &entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            for (auto &c : ext) c = tolower(c);

            if (ext == ".glb" || ext == ".obj" || ext == ".fbx" || ext == ".gltf") {
                // Percorso relativo da assets/
                std::string relPath = entry.path().string();
                m_modelFiles.push_back(relPath);
            }
        }

        std::sort(m_modelFiles.begin(), m_modelFiles.end());
    } catch (const std::filesystem::filesystem_error &) {
        // Directory non accessibile
    }
}

int EnvironmentalObject::findOrCreatePatch(RockMeshType type)
{
    // Non cercare patch CUSTOM per tipo, solo per primitivi
    if (type == RockMeshType::CUSTOM) return -1;

    for (int i = 0; i < (int)m_patches.size(); i++) {
        if (m_patches[i].meshType == type) return i;
    }
    return addPatch(type);
}

int EnvironmentalObject::addPatch(RockMeshType type)
{
    loadShader();

    RockPatch patch;
    patch.meshType = type;
    patch.mesh = generateMesh(type, m_rockSize);
    patch.material = LoadMaterialDefault();
    patch.material.shader = m_instancingShader;

    Color colors[] = {
        {180, 210, 50, 255},   // giallo-verde (cube)
        {100, 180, 220, 255},  // azzurro (sphere)
        {200, 140, 60, 255},   // arancione (hemisphere)
        {160, 160, 170, 255},  // grigio (cylinder)
        {190, 100, 130, 255},  // rosa (cone)
    };
    int ci = (int)type < 5 ? (int)type : 0;
    patch.material.maps[MATERIAL_MAP_DIFFUSE].color = colors[ci];

    m_patches.push_back(std::move(patch));
    int idx = (int)m_patches.size() - 1;

    TraceLog(LOG_INFO, "Rocks: Created patch %d (%s)", idx, rockMeshTypeName(type));
    return idx;
}

int EnvironmentalObject::addPatchFromModel(const std::string &modelPath)
{
    loadShader();

    Model model = LoadModel(modelPath.c_str());
    if (model.meshCount == 0) {
        TraceLog(LOG_WARNING, "Rocks: Failed to load model %s", modelPath.c_str());
        UnloadModel(model);
        return -1;
    }

    // Copia la prima mesh del modello
    // Raylib non ha un "copy mesh" diretto, quindi usiamo GenMeshCopy
    // via upload: prendiamo i dati dalla mesh caricata
    Mesh srcMesh = model.meshes[0];

    // Alloca una nuova mesh copiando i dati vertex
    Mesh mesh = {0};
    mesh.vertexCount = srcMesh.vertexCount;
    mesh.triangleCount = srcMesh.triangleCount;

    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
    memcpy(mesh.vertices, srcMesh.vertices, mesh.vertexCount * 3 * sizeof(float));

    if (srcMesh.texcoords) {
        mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
        memcpy(mesh.texcoords, srcMesh.texcoords, mesh.vertexCount * 2 * sizeof(float));
    }

    if (srcMesh.normals) {
        mesh.normals = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
        memcpy(mesh.normals, srcMesh.normals, mesh.vertexCount * 3 * sizeof(float));
    }

    if (srcMesh.indices) {
        mesh.indices = (unsigned short *)RL_MALLOC(mesh.triangleCount * 3 * sizeof(unsigned short));
        memcpy(mesh.indices, srcMesh.indices, mesh.triangleCount * 3 * sizeof(unsigned short));
    }

    UploadMesh(&mesh, false);

    // Estrai il nome del file
    std::filesystem::path p(modelPath);
    std::string fileName = p.stem().string();

    RockPatch patch;
    patch.meshType = RockMeshType::CUSTOM;
    patch.customName = fileName;
    patch.mesh = mesh;
    patch.material = LoadMaterialDefault();
    patch.material.shader = m_instancingShader;
    patch.material.maps[MATERIAL_MAP_DIFFUSE].color = {220, 220, 220, 255};

    // Se il modello ha texture, copiale nel material
    if (model.materialCount > 0 && model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.id != 0) {
        patch.material.maps[MATERIAL_MAP_DIFFUSE].texture =
            model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture;
        // Non fare UnloadModel perche' scaricherebbe la texture
        // Libera solo le mesh (tranne la prima che abbiamo copiato) e il model struct
    }

    UnloadModel(model);

    m_patches.push_back(std::move(patch));
    int idx = (int)m_patches.size() - 1;

    TraceLog(LOG_INFO, "Rocks: Created custom patch %d from %s (%d verts)",
             idx, fileName.c_str(), mesh.vertexCount);
    return idx;
}

void EnvironmentalObject::generate(const Model &terrain, int count, RockMeshType type)
{
    m_terrain = &terrain;
    m_initialized = true;

    int patchIdx = findOrCreatePatch(type);
    if (patchIdx < 0) return;
    auto &patch = m_patches[patchIdx];

    BoundingBox bounds = GetMeshBoundingBox(terrain.meshes[0]);
    Vector3 boundsMin = Vector3Transform(bounds.min, terrain.transform);
    Vector3 boundsMax = Vector3Transform(bounds.max, terrain.transform);

    float minX = fmaxf(boundsMin.x, -m_spawnRadius);
    float maxX = fminf(boundsMax.x, m_spawnRadius);
    float minZ = fmaxf(boundsMin.z, -m_spawnRadius);
    float maxZ = fminf(boundsMax.z, m_spawnRadius);

    srand(42 + (int)type);

    int placed = 0;
    for (int i = 0; i < count; i++) {
        float x = minX + ((float)rand() / RAND_MAX) * (maxX - minX);
        float z = minZ + ((float)rand() / RAND_MAX) * (maxZ - minZ);

        Ray ray;
        ray.position = {x, 1000.0f, z};
        ray.direction = {0.0f, -1.0f, 0.0f};

        float y = 0.0f;
        bool onGround = false;
        for (int m = 0; m < terrain.meshCount; m++) {
            RayCollision hit = GetRayCollisionMesh(ray, terrain.meshes[m], terrain.transform);
            if (hit.hit) {
                y = hit.point.y;
                onGround = true;
                break;
            }
        }

        if (!onGround || y < 0.5f) continue;

        float scaleVar = 0.5f + ((float)rand() / RAND_MAX) * 1.5f;
        float rotY = ((float)rand() / RAND_MAX) * 360.0f * DEG2RAD;
        float rotX = ((float)rand() / RAND_MAX) * 15.0f * DEG2RAD;
        float rotZ = ((float)rand() / RAND_MAX) * 15.0f * DEG2RAD;

        y -= m_rockSize * scaleVar * 0.15f;

        Matrix matScale = MatrixScale(scaleVar, scaleVar * 0.6f, scaleVar);
        Matrix matRotation = MatrixMultiply(
            MatrixMultiply(MatrixRotateX(rotX), MatrixRotateY(rotY)),
            MatrixRotateZ(rotZ));
        Matrix matTranslation = MatrixTranslate(x, y, z);
        Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

        patch.transforms.push_back(transform);
        placed++;
    }

    m_activePatch = patchIdx;

    TraceLog(LOG_INFO, "Rocks: Generated %d instances in patch %d (%s)",
             placed, patchIdx, rockMeshTypeName(type));
}

void EnvironmentalObject::setActivePatch(int idx)
{
    if (idx >= 0 && idx < (int)m_patches.size()) {
        m_activePatch = idx;
    }
}

RockMeshType EnvironmentalObject::getActiveMeshType() const
{
    if (m_activePatch >= 0 && m_activePatch < (int)m_patches.size()) {
        return m_patches[m_activePatch].meshType;
    }
    return RockMeshType::CUBE;
}

void EnvironmentalObject::setActiveMeshType(RockMeshType type)
{
    if (type == RockMeshType::CUSTOM) return; // Usa addPatchFromModel
    int idx = findOrCreatePatch(type);
    if (idx >= 0) m_activePatch = idx;
}

void EnvironmentalObject::paintAt(Vector3 center)
{
    if (!m_initialized || !m_terrain) return;
    if (m_patches.empty()) return;

    auto &patch = m_patches[m_activePatch];

    for (int i = 0; i < m_brushDensity; i++) {
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float dist = sqrtf((float)rand() / RAND_MAX) * m_brushRadius;
        float x = center.x + cosf(angle) * dist;
        float z = center.z + sinf(angle) * dist;

        Ray ray;
        ray.position = {x, 1000.0f, z};
        ray.direction = {0.0f, -1.0f, 0.0f};

        float y = 0.0f;
        bool onGround = false;
        for (int m = 0; m < m_terrain->meshCount; m++) {
            RayCollision hit = GetRayCollisionMesh(ray, m_terrain->meshes[m], m_terrain->transform);
            if (hit.hit) {
                y = hit.point.y;
                onGround = true;
                break;
            }
        }

        if (!onGround || y < 0.5f) continue;

        float scaleVar = 0.5f + ((float)rand() / RAND_MAX) * 1.5f;
        float rotY = ((float)rand() / RAND_MAX) * 360.0f * DEG2RAD;
        float rotX = ((float)rand() / RAND_MAX) * 15.0f * DEG2RAD;
        float rotZ = ((float)rand() / RAND_MAX) * 15.0f * DEG2RAD;

        y -= m_rockSize * scaleVar * 0.15f;

        Matrix matScale = MatrixScale(scaleVar, scaleVar * 0.6f, scaleVar);
        Matrix matRotation = MatrixMultiply(
            MatrixMultiply(MatrixRotateX(rotX), MatrixRotateY(rotY)),
            MatrixRotateZ(rotZ));
        Matrix matTranslation = MatrixTranslate(x, y, z);
        Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

        patch.transforms.push_back(transform);
    }
}

void EnvironmentalObject::eraseAt(Vector3 center)
{
    if (!m_initialized) return;

    float r2 = m_brushRadius * m_brushRadius;

    for (auto &patch : m_patches) {
        patch.transforms.erase(
            std::remove_if(patch.transforms.begin(), patch.transforms.end(),
                [&](const Matrix &mat) {
                    float dx = mat.m12 - center.x;
                    float dz = mat.m14 - center.z;
                    return (dx * dx + dz * dz) <= r2;
                }),
            patch.transforms.end());
    }
}

void EnvironmentalObject::clearAll()
{
    for (auto &patch : m_patches) {
        patch.transforms.clear();
    }
}

int EnvironmentalObject::getTotalInstanceCount() const
{
    int total = 0;
    for (auto &patch : m_patches) {
        total += (int)patch.transforms.size();
    }
    return total;
}

void EnvironmentalObject::draw()
{
    if (!isVisible || !m_initialized) return;

    float cullDist2 = m_cullDistance * m_cullDistance;

    for (auto &patch : m_patches) {
        if (patch.transforms.empty()) continue;

        m_visibleBuffer.clear();
        for (auto &t : patch.transforms) {
            float dx = t.m12 - m_cameraPos.x;
            float dz = t.m14 - m_cameraPos.z;
            if ((dx * dx + dz * dz) <= cullDist2) {
                m_visibleBuffer.push_back(t);
            }
        }

        if (!m_visibleBuffer.empty()) {
            DrawMeshInstanced(patch.mesh, patch.material,
                              m_visibleBuffer.data(), (int)m_visibleBuffer.size());
        }
    }
}

void EnvironmentalObject::gui()
{
    ImGui::PushID(this);
    if (ImGui::CollapsingHeader("Rocks (Instanced)")) {
        ImGui::Text("Total instances: %d", getTotalInstanceCount());
        ImGui::Text("Patches: %d", (int)m_patches.size());
        ImGui::Text("Cull distance: %.0f", m_cullDistance);
        ImGui::Checkbox("Visible", &isVisible);

        for (int i = 0; i < (int)m_patches.size(); i++) {
            auto &p = m_patches[i];
            ImGui::Text("  [%d] %s: %d instances", i,
                        patchDisplayName(p), (int)p.transforms.size());
        }
    }
    ImGui::PopID();
}

} // namespace moiras
