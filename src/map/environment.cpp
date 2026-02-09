#include "environment.hpp"
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace moiras {

const char *rockMeshTypeName(RockMeshType type)
{
    switch (type) {
        case RockMeshType::CUBE:       return "Cube";
        case RockMeshType::SPHERE:     return "Sphere";
        case RockMeshType::HEMISPHERE: return "Hemisphere";
        case RockMeshType::CYLINDER:   return "Cylinder";
        case RockMeshType::CONE:       return "Cone";
        default:                       return "Unknown";
    }
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

int EnvironmentalObject::findOrCreatePatch(RockMeshType type)
{
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

    // Colori diversi per patch per distinguerli visivamente
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

void EnvironmentalObject::generate(const Model &terrain, int count, RockMeshType type)
{
    m_terrain = &terrain;
    m_initialized = true;

    int patchIdx = findOrCreatePatch(type);
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
    int idx = findOrCreatePatch(type);
    m_activePatch = idx;
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

    // Cancella da tutti i patch nel raggio
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

        // Filtra le istanze visibili entro la distanza di culling
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
                        rockMeshTypeName(p.meshType), (int)p.transforms.size());
        }
    }
    ImGui::PopID();
}

} // namespace moiras
