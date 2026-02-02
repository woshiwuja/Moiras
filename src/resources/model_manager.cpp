#include "model_manager.h"
#include "imgui.h"
#include <raylib.h>
#include <cstring>

namespace moiras {

// ============================================================================
// ModelInstance implementation
// ============================================================================

ModelInstance::ModelInstance()
    : m_manager(nullptr), m_sharedMeshes(nullptr), m_meshCount(0),
      m_meshMaterial(nullptr), m_localMeshes(nullptr),
      m_bones(nullptr), m_boneCount(0),
      m_bindPose(nullptr), m_currentPose(nullptr),
      m_materials(nullptr), m_materialCount(0) {}

ModelInstance::ModelInstance(ModelManager* manager, const std::string& path,
                             Mesh* meshes, int meshCount,
                             int* meshMaterial, Material* sourceMaterials, int materialCount,
                             BoneInfo* bones, int boneCount, Transform* bindPose)
    : m_manager(manager), m_path(path),
      m_sharedMeshes(meshes), m_meshCount(meshCount), m_meshMaterial(meshMaterial),
      m_localMeshes(nullptr),
      m_bones(bones), m_boneCount(boneCount), m_bindPose(bindPose),
      m_currentPose(nullptr),
      m_materials(nullptr), m_materialCount(materialCount) {

    // Clone materials array for per-instance shader support
    if (materialCount > 0 && sourceMaterials != nullptr) {
        m_materials = static_cast<Material*>(RL_MALLOC(materialCount * sizeof(Material)));
        for (int i = 0; i < materialCount; i++) {
            m_materials[i] = sourceMaterials[i];
        }
    }

    // Clone bind pose to per-instance current pose for animations
    if (boneCount > 0 && bindPose != nullptr) {
        m_currentPose = static_cast<Transform*>(RL_MALLOC(boneCount * sizeof(Transform)));
        for (int i = 0; i < boneCount; i++) {
            m_currentPose[i] = bindPose[i];
        }
    }
}

ModelInstance::~ModelInstance() {
    release();
}

ModelInstance::ModelInstance(ModelInstance&& other) noexcept
    : m_manager(other.m_manager), m_path(std::move(other.m_path)),
      m_sharedMeshes(other.m_sharedMeshes), m_meshCount(other.m_meshCount),
      m_meshMaterial(other.m_meshMaterial),
      m_localMeshes(other.m_localMeshes),
      m_bones(other.m_bones), m_boneCount(other.m_boneCount),
      m_bindPose(other.m_bindPose), m_currentPose(other.m_currentPose),
      m_materials(other.m_materials), m_materialCount(other.m_materialCount),
      m_animData(std::move(other.m_animData)) {

    // Clear other to prevent double-release
    other.m_manager = nullptr;
    other.m_sharedMeshes = nullptr;
    other.m_meshCount = 0;
    other.m_meshMaterial = nullptr;
    other.m_localMeshes = nullptr;
    other.m_bones = nullptr;
    other.m_boneCount = 0;
    other.m_bindPose = nullptr;
    other.m_currentPose = nullptr;
    other.m_materials = nullptr;
    other.m_materialCount = 0;
}

ModelInstance& ModelInstance::operator=(ModelInstance&& other) noexcept {
    if (this != &other) {
        release();

        m_manager = other.m_manager;
        m_path = std::move(other.m_path);
        m_sharedMeshes = other.m_sharedMeshes;
        m_meshCount = other.m_meshCount;
        m_meshMaterial = other.m_meshMaterial;
        m_localMeshes = other.m_localMeshes;
        m_bones = other.m_bones;
        m_boneCount = other.m_boneCount;
        m_bindPose = other.m_bindPose;
        m_currentPose = other.m_currentPose;
        m_materials = other.m_materials;
        m_materialCount = other.m_materialCount;
        m_animData = std::move(other.m_animData);

        other.m_manager = nullptr;
        other.m_sharedMeshes = nullptr;
        other.m_meshCount = 0;
        other.m_meshMaterial = nullptr;
        other.m_localMeshes = nullptr;
        other.m_bones = nullptr;
        other.m_boneCount = 0;
        other.m_bindPose = nullptr;
        other.m_currentPose = nullptr;
        other.m_materials = nullptr;
        other.m_materialCount = 0;
    }
    return *this;
}

void ModelInstance::releaseAnimationData() {
    // Free all per-instance animation data
    for (auto& data : m_animData) {
        if (data.animVertices != nullptr) {
            RL_FREE(data.animVertices);
        }
        if (data.animNormals != nullptr) {
            RL_FREE(data.animNormals);
        }
        if (data.boneMatrices != nullptr) {
            RL_FREE(data.boneMatrices);
        }
    }
    m_animData.clear();

    // Free per-instance mesh structures
    if (m_localMeshes != nullptr) {
        RL_FREE(m_localMeshes);
        m_localMeshes = nullptr;
    }
}

void ModelInstance::release() {
    // Free per-instance animation data first
    releaseAnimationData();

    // Free per-instance materials (but not the textures, those are shared)
    if (m_materials != nullptr) {
        RL_FREE(m_materials);
        m_materials = nullptr;
    }
    m_materialCount = 0;

    // Free per-instance current pose
    if (m_currentPose != nullptr) {
        RL_FREE(m_currentPose);
        m_currentPose = nullptr;
    }

    // Tell manager to decrement ref count
    if (m_manager != nullptr && !m_path.empty()) {
        m_manager->release(m_path);
    }

    m_manager = nullptr;
    m_sharedMeshes = nullptr;
    m_meshCount = 0;
    m_meshMaterial = nullptr;
    m_bones = nullptr;
    m_boneCount = 0;
    m_bindPose = nullptr;
    m_path.clear();
}

void ModelInstance::resetPose() {
    if (m_currentPose != nullptr && m_bindPose != nullptr && m_boneCount > 0) {
        for (int i = 0; i < m_boneCount; i++) {
            m_currentPose[i] = m_bindPose[i];
        }
    }
}

void ModelInstance::prepareForAnimation() {
    if (!isValid() || m_boneCount == 0) {
        return; // No bones = no animation support
    }

    // Already prepared?
    if (m_localMeshes != nullptr) {
        return;
    }

    // Clone mesh structures (not the vertex data, just the Mesh structs)
    // This allows each instance to have its own bone matrix buffers
    m_localMeshes = static_cast<Mesh*>(RL_MALLOC(m_meshCount * sizeof(Mesh)));
    m_animData.resize(m_meshCount);

    for (int i = 0; i < m_meshCount; i++) {
        // Copy the entire Mesh structure (pointers to shared VBOs, etc.)
        m_localMeshes[i] = m_sharedMeshes[i];

        Mesh& localMesh = m_localMeshes[i];
        MeshAnimationData& data = m_animData[i];

        data.vertexCount = localMesh.vertexCount;
        data.boneCount = localMesh.boneCount;

        // Null out CPU skinning buffers — GPU skinning via bone matrices is used
        // instead. This prevents UpdateModelAnimation from doing expensive
        // per-vertex CPU transforms and large buffer uploads every frame.
        localMesh.animVertices = nullptr;
        localMesh.animNormals = nullptr;

        // Clone boneMatrices for per-instance animation (GPU skinning path)
        if (m_sharedMeshes[i].boneMatrices != nullptr && localMesh.boneCount > 0) {
            size_t size = localMesh.boneCount * sizeof(Matrix);
            data.boneMatrices = static_cast<Matrix*>(RL_MALLOC(size));
            memcpy(data.boneMatrices, m_sharedMeshes[i].boneMatrices, size);
            // Point local mesh to our per-instance buffer
            localMesh.boneMatrices = data.boneMatrices;
        }
    }

    TraceLog(LOG_INFO, "ModelInstance: Prepared per-instance animation data (GPU skinning) for %d meshes", m_meshCount);
}

void ModelInstance::bindAnimationData() {
    // No-op: with per-instance mesh structures, animation buffers are always ready
    // This function is kept for API compatibility
}

void ModelInstance::unbindAnimationData() {
    // No-op: with per-instance mesh structures, no need to restore shared state
    // This function is kept for API compatibility
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

    if (m_sharedMeshes != nullptr && m_meshCount > 0) {
        bounds = GetMeshBoundingBox(m_sharedMeshes[0]);

        for (int i = 1; i < m_meshCount; i++) {
            BoundingBox meshBounds = GetMeshBoundingBox(m_sharedMeshes[i]);

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
