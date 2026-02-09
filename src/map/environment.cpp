#include "environment.hpp"
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <cstdlib>

namespace moiras {

EnvironmentalObject::EnvironmentalObject(int instanceCount, float rockSize, float spawnRadius)
    : GameObject("Rocks"),
      m_rockMesh{0},
      m_material{0},
      m_instanceCount(instanceCount),
      m_rockSize(rockSize),
      m_spawnRadius(spawnRadius),
      m_initialized(false)
{
}

EnvironmentalObject::~EnvironmentalObject()
{
    if (m_initialized) {
        UnloadMesh(m_rockMesh);
        UnloadMaterial(m_material);
    }
}

void EnvironmentalObject::generate(const Model &terrain)
{
    if (m_initialized) {
        UnloadMesh(m_rockMesh);
        UnloadMaterial(m_material);
    }

    // Genera mesh cubica per le rocce
    m_rockMesh = GenMeshCube(m_rockSize, m_rockSize * 0.7f, m_rockSize);

    // Materiale base grigio per le rocce
    m_material = LoadMaterialDefault();
    m_material.maps[MATERIAL_MAP_DIFFUSE].color = {120, 110, 100, 255};

    // Calcola i limiti del terreno
    BoundingBox bounds = GetMeshBoundingBox(terrain.meshes[0]);
    // Applica la trasformazione del modello ai bounds
    Vector3 boundsMin = Vector3Transform(bounds.min, terrain.transform);
    Vector3 boundsMax = Vector3Transform(bounds.max, terrain.transform);

    // Limita l'area di spawn al raggio configurato
    float minX = fmaxf(boundsMin.x, -m_spawnRadius);
    float maxX = fminf(boundsMax.x, m_spawnRadius);
    float minZ = fmaxf(boundsMin.z, -m_spawnRadius);
    float maxZ = fminf(boundsMax.z, m_spawnRadius);

    m_transforms.clear();
    m_transforms.reserve(m_instanceCount);

    srand(42); // Seed fisso per risultati riproducibili

    for (int i = 0; i < m_instanceCount; i++) {
        // Posizione casuale nell'area di spawn
        float x = minX + ((float)rand() / RAND_MAX) * (maxX - minX);
        float z = minZ + ((float)rand() / RAND_MAX) * (maxZ - minZ);

        // Raycast verso il basso per trovare l'altezza del terreno
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

        // Salta rocce che finirebbero sotto il livello del mare
        if (!onGround || y < 0.5f) continue;

        // Scala e rotazione casuali per varieta' visiva
        float scaleVar = 0.5f + ((float)rand() / RAND_MAX) * 1.5f;
        float rotY = ((float)rand() / RAND_MAX) * 360.0f * DEG2RAD;
        float rotX = ((float)rand() / RAND_MAX) * 15.0f * DEG2RAD;
        float rotZ = ((float)rand() / RAND_MAX) * 15.0f * DEG2RAD;

        // Offset Y per interrare leggermente la roccia
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

    TraceLog(LOG_INFO, "Rocks: Generated %d instanced rocks (GPU instancing)", m_instanceCount);
}

void EnvironmentalObject::setShader(Shader shader)
{
    if (m_initialized) {
        m_material.shader = shader;
    }
}

void EnvironmentalObject::draw()
{
    if (!isVisible || !m_initialized || m_transforms.empty()) return;

    DrawMeshInstanced(m_rockMesh, m_material, m_transforms.data(), m_instanceCount);
}

void EnvironmentalObject::gui()
{
    ImGui::PushID(this);
    if (ImGui::CollapsingHeader("Rocks (Instanced)")) {
        ImGui::Text("Instances: %d", m_instanceCount);
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
