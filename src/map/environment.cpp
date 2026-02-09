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

EnvironmentalObject::EnvironmentalObject(int instanceCount, float rockSize, float spawnRadius)
    : GameObject("Rocks"),
      m_rockMesh{0},
      m_material{0},
      m_currentShader{0},
      m_terrain(nullptr),
      m_instanceCount(instanceCount),
      m_targetInstanceCount(instanceCount),
      m_rockSize(rockSize),
      m_spawnRadius(spawnRadius),
      m_initialized(false),
      m_hasShader(false),
      m_meshType(RockMeshType::CUBE),
      m_brushMode(false),
      m_brushRadius(10.0f),
      m_brushDensity(5)
{
}

EnvironmentalObject::~EnvironmentalObject()
{
    if (m_initialized) {
        UnloadMesh(m_rockMesh);
        UnloadMaterial(m_material);
    }
    if (m_hasShader) {
        UnloadShader(m_currentShader);
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

void EnvironmentalObject::generate(const Model &terrain)
{
    if (m_initialized) {
        UnloadMesh(m_rockMesh);
        UnloadMaterial(m_material);
    }

    m_terrain = &terrain;
    m_instanceCount = m_targetInstanceCount;

    m_rockMesh = generateMesh(m_meshType, m_rockSize);

    // Carica shader di instancing (richiesto da DrawMeshInstanced)
    if (!m_hasShader) {
        m_currentShader = LoadShader("../assets/shaders/instancing.vs",
                                     "../assets/shaders/instancing.fs");
        m_currentShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(m_currentShader, "mvp");
        m_currentShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(m_currentShader, "instanceTransform");
        m_hasShader = true;
    }

    m_material = LoadMaterialDefault();
    m_material.shader = m_currentShader;
    m_material.maps[MATERIAL_MAP_DIFFUSE].color = {180, 210, 50, 255};

    BoundingBox bounds = GetMeshBoundingBox(terrain.meshes[0]);
    Vector3 boundsMin = Vector3Transform(bounds.min, terrain.transform);
    Vector3 boundsMax = Vector3Transform(bounds.max, terrain.transform);

    float minX = fmaxf(boundsMin.x, -m_spawnRadius);
    float maxX = fminf(boundsMax.x, m_spawnRadius);
    float minZ = fmaxf(boundsMin.z, -m_spawnRadius);
    float maxZ = fminf(boundsMax.z, m_spawnRadius);

    m_transforms.clear();
    m_transforms.reserve(m_instanceCount);

    srand(42);

    for (int i = 0; i < m_instanceCount; i++) {
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

        m_transforms.push_back(transform);
    }

    m_instanceCount = (int)m_transforms.size();
    m_initialized = true;

    TraceLog(LOG_INFO, "Rocks: Generated %d instanced rocks (mesh: %s)",
             m_instanceCount, rockMeshTypeName(m_meshType));
}

void EnvironmentalObject::setShader(Shader shader)
{
    (void)shader;
}

void EnvironmentalObject::setMeshType(RockMeshType type)
{
    if (type == m_meshType) return;
    m_meshType = type;

    if (m_initialized) {
        UnloadMesh(m_rockMesh);
        m_rockMesh = generateMesh(m_meshType, m_rockSize);
        TraceLog(LOG_INFO, "Rocks: Mesh changed to %s", rockMeshTypeName(m_meshType));
    }
}

void EnvironmentalObject::paintAt(Vector3 center)
{
    if (!m_initialized || !m_terrain) return;

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

        m_transforms.push_back(transform);
    }

    m_instanceCount = (int)m_transforms.size();
}

void EnvironmentalObject::eraseAt(Vector3 center)
{
    if (!m_initialized || m_transforms.empty()) return;

    float r2 = m_brushRadius * m_brushRadius;

    m_transforms.erase(
        std::remove_if(m_transforms.begin(), m_transforms.end(),
            [&](const Matrix &mat) {
                float dx = mat.m12 - center.x;
                float dz = mat.m14 - center.z;
                return (dx * dx + dz * dz) <= r2;
            }),
        m_transforms.end());

    m_instanceCount = (int)m_transforms.size();
}

void EnvironmentalObject::clearAll()
{
    m_transforms.clear();
    m_instanceCount = 0;
}

void EnvironmentalObject::draw()
{
    if (!isVisible || !m_initialized) return;

    if (!m_transforms.empty()) {
        DrawMeshInstanced(m_rockMesh, m_material, m_transforms.data(), m_instanceCount);
    }
}

void EnvironmentalObject::gui()
{
    ImGui::PushID(this);
    if (ImGui::CollapsingHeader("Rocks (Instanced)")) {
        ImGui::Text("Instances: %d", m_instanceCount);
        ImGui::Text("Mesh: %s", rockMeshTypeName(m_meshType));
        ImGui::Text("GPU Instancing: Active");
        ImGui::Checkbox("Visible", &isVisible);

        Color &col = m_material.maps[MATERIAL_MAP_DIFFUSE].color;
        float color[3] = {col.r / 255.0f, col.g / 255.0f, col.b / 255.0f};
        if (ImGui::ColorEdit3("Rock Color", color)) {
            col.r = (unsigned char)(color[0] * 255.0f);
            col.g = (unsigned char)(color[1] * 255.0f);
            col.b = (unsigned char)(color[2] * 255.0f);
        }
    }
    ImGui::PopID();
}

} // namespace moiras
