#include "model_manager.h"
#include "imgui.h"
#include <raylib.h>

namespace moiras {

// ============================================================================
// ModelInstance implementation
// ============================================================================

ModelInstance::ModelInstance()
    : m_manager(nullptr), m_meshes(nullptr), m_meshCount(0),
      m_meshMaterial(nullptr), m_bones(nullptr), m_boneCount(0),
      m_bindPose(nullptr), m_materials(nullptr), m_materialCount(0) {}

ModelInstance::ModelInstance(ModelManager* manager, const std::string& path,
                             Mesh* meshes, int meshCount,
                             int* meshMaterial, Material* sourceMaterials, int materialCount,
                             BoneInfo* bones, int boneCount, Transform* bindPose)
    : m_manager(manager), m_path(path),
      m_meshes(meshes), m_meshCount(meshCount), m_meshMaterial(meshMaterial),
      m_bones(bones), m_boneCount(boneCount), m_bindPose(bindPose),
      m_materials(nullptr), m_materialCount(materialCount) {

    // Clone materials array for per-instance shader support
    if (materialCount > 0 && sourceMaterials != nullptr) {
        m_materials = static_cast<Material*>(RL_MALLOC(materialCount * sizeof(Material)));
        for (int i = 0; i < materialCount; i++) {
            m_materials[i] = sourceMaterials[i];
        }
    }
}

ModelInstance::~ModelInstance() {
    release();
}

ModelInstance::ModelInstance(ModelInstance&& other) noexcept
    : m_manager(other.m_manager), m_path(std::move(other.m_path)),
      m_meshes(other.m_meshes), m_meshCount(other.m_meshCount),
      m_meshMaterial(other.m_meshMaterial),
      m_bones(other.m_bones), m_boneCount(other.m_boneCount),
      m_bindPose(other.m_bindPose),
      m_materials(other.m_materials), m_materialCount(other.m_materialCount) {

    // Clear other to prevent double-release
    other.m_manager = nullptr;
    other.m_meshes = nullptr;
    other.m_meshCount = 0;
    other.m_meshMaterial = nullptr;
    other.m_bones = nullptr;
    other.m_boneCount = 0;
    other.m_bindPose = nullptr;
    other.m_materials = nullptr;
    other.m_materialCount = 0;
}

ModelInstance& ModelInstance::operator=(ModelInstance&& other) noexcept {
    if (this != &other) {
        release();

        m_manager = other.m_manager;
        m_path = std::move(other.m_path);
        m_meshes = other.m_meshes;
        m_meshCount = other.m_meshCount;
        m_meshMaterial = other.m_meshMaterial;
        m_bones = other.m_bones;
        m_boneCount = other.m_boneCount;
        m_bindPose = other.m_bindPose;
        m_materials = other.m_materials;
        m_materialCount = other.m_materialCount;

        other.m_manager = nullptr;
        other.m_meshes = nullptr;
        other.m_meshCount = 0;
        other.m_meshMaterial = nullptr;
        other.m_bones = nullptr;
        other.m_boneCount = 0;
        other.m_bindPose = nullptr;
        other.m_materials = nullptr;
        other.m_materialCount = 0;
    }
    return *this;
}

void ModelInstance::release() {
    // Free per-instance materials (but not the textures, those are shared)
    if (m_materials != nullptr) {
        RL_FREE(m_materials);
        m_materials = nullptr;
    }
    m_materialCount = 0;

    // Tell manager to decrement ref count
    if (m_manager != nullptr && !m_path.empty()) {
        m_manager->release(m_path);
    }

    m_manager = nullptr;
    m_meshes = nullptr;
    m_meshCount = 0;
    m_meshMaterial = nullptr;
    m_bones = nullptr;
    m_boneCount = 0;
    m_bindPose = nullptr;
    m_path.clear();
}

void ModelInstance::applyShader(Shader shader) {
    if (m_materials != nullptr && shader.id > 0) {
        for (int i = 0; i < m_materialCount; i++) {
            m_materials[i].shader = shader;
        }
        TraceLog(LOG_INFO, "ModelInstance: Applied shader %d to %d materials",
                 shader.id, m_materialCount);
    }
}

BoundingBox ModelInstance::getBoundingBox() const {
    BoundingBox bounds = {0};

    if (m_meshes != nullptr && m_meshCount > 0) {
        bounds = GetMeshBoundingBox(m_meshes[0]);

        for (int i = 1; i < m_meshCount; i++) {
            BoundingBox meshBounds = GetMeshBoundingBox(m_meshes[i]);

            // Expand bounds to include this mesh
            if (meshBounds.min.x < bounds.min.x) bounds.min.x = meshBounds.min.x;
            if (meshBounds.min.y < bounds.min.y) bounds.min.y = meshBounds.min.y;
            if (meshBounds.min.z < bounds.min.z) bounds.min.z = meshBounds.min.z;
            if (meshBounds.max.x > bounds.max.x) bounds.max.x = meshBounds.max.x;
            if (meshBounds.max.y > bounds.max.y) bounds.max.y = meshBounds.max.y;
            if (meshBounds.max.z > bounds.max.z) bounds.max.z = meshBounds.max.z;
        }
    }

    return bounds;
}

// ============================================================================
// ModelManager implementation
// ============================================================================

ModelManager::ModelManager() {}

ModelManager::~ModelManager() {
    unloadAll();
}

ModelInstance ModelManager::acquire(const std::string& path) {
    auto it = m_cache.find(path);

    if (it == m_cache.end()) {
        // Load new model
        Model model = LoadModel(path.c_str());

        if (model.meshCount == 0) {
            TraceLog(LOG_ERROR, "ModelManager: Failed to load model: %s", path.c_str());
            return ModelInstance();
        }

        TraceLog(LOG_INFO, "ModelManager: Loaded model '%s' (%d meshes, %d materials)",
                 path.c_str(), model.meshCount, model.materialCount);

        m_cache[path] = CachedModel{model, 1};
        it = m_cache.find(path);
    } else {
        it->second.refCount++;
        TraceLog(LOG_INFO, "ModelManager: Reusing cached model '%s' (refCount: %d)",
                 path.c_str(), it->second.refCount);
    }

    CachedModel& cached = it->second;
    return ModelInstance(this, path,
                         cached.model.meshes, cached.model.meshCount,
                         cached.model.meshMaterial,
                         cached.model.materials, cached.model.materialCount,
                         cached.model.bones, cached.model.boneCount,
                         cached.model.bindPose);
}

void ModelManager::preload(const std::string& path) {
    if (m_cache.find(path) != m_cache.end()) {
        return; // Already cached
    }

    Model model = LoadModel(path.c_str());

    if (model.meshCount == 0) {
        TraceLog(LOG_ERROR, "ModelManager: Failed to preload model: %s", path.c_str());
        return;
    }

    TraceLog(LOG_INFO, "ModelManager: Preloaded model '%s'", path.c_str());
    m_cache[path] = CachedModel{model, 0}; // refCount 0 for preloaded
}

void ModelManager::release(const std::string& path) {
    auto it = m_cache.find(path);

    if (it == m_cache.end()) {
        TraceLog(LOG_WARNING, "ModelManager: Tried to release unknown model: %s", path.c_str());
        return;
    }

    it->second.refCount--;
    TraceLog(LOG_INFO, "ModelManager: Released model '%s' (refCount: %d)",
             path.c_str(), it->second.refCount);

    if (it->second.refCount <= 0) {
        TraceLog(LOG_INFO, "ModelManager: Unloading unused model '%s'", path.c_str());
        UnloadModel(it->second.model);
        m_cache.erase(it);
    }
}

int ModelManager::getRefCount(const std::string& path) const {
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        return it->second.refCount;
    }
    return 0;
}

void ModelManager::unloadAll() {
    for (auto& pair : m_cache) {
        TraceLog(LOG_INFO, "ModelManager: Unloading model '%s'", pair.first.c_str());
        UnloadModel(pair.second.model);
    }
    m_cache.clear();
}

void ModelManager::gui() {
    if (ImGui::CollapsingHeader("Model Manager")) {
        ImGui::Text("Cached Models: %d", static_cast<int>(m_cache.size()));
        ImGui::Separator();

        for (const auto& pair : m_cache) {
            ImGui::Text("%s", pair.first.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(refs: %d, meshes: %d)",
                                pair.second.refCount,
                                pair.second.model.meshCount);
        }
    }
}

} // namespace moiras
