#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace moiras {

class ModelManager;

// Per-mesh animation data for independent animation playback
struct MeshAnimationData {
    float* animVertices = nullptr;   // Per-instance animated vertices
    float* animNormals = nullptr;    // Per-instance animated normals
    Matrix* boneMatrices = nullptr;  // Per-instance bone matrices
    int vertexCount = 0;
    int boneCount = 0;
};

// ModelInstance - holds a reference to shared mesh data with per-instance materials
class ModelInstance {
public:
    ModelInstance();
    ~ModelInstance();

    // Move semantics
    ModelInstance(ModelInstance&& other) noexcept;
    ModelInstance& operator=(ModelInstance&& other) noexcept;

    // No copy (would mess up ref counting)
    ModelInstance(const ModelInstance&) = delete;
    ModelInstance& operator=(const ModelInstance&) = delete;

    // Check if valid
    bool isValid() const { return m_sharedMeshes != nullptr && m_meshCount > 0; }

    // Access mesh data (returns local meshes if prepared for animation, otherwise shared)
    Mesh* meshes() const { return m_localMeshes != nullptr ? m_localMeshes : m_sharedMeshes; }
    int meshCount() const { return m_meshCount; }
    int* meshMaterial() const { return m_meshMaterial; }

    // Access bone data for animations
    BoneInfo* bones() const { return m_bones; }
    int boneCount() const { return m_boneCount; }
    Transform* bindPose() const { return m_bindPose; }

    // Access per-instance current pose (for animations)
    Transform* currentPose() { return m_currentPose; }
    const Transform* currentPose() const { return m_currentPose; }

    // Access per-instance materials
    Material* materials() { return m_materials; }
    const Material* materials() const { return m_materials; }
    int materialCount() const { return m_materialCount; }

    // Apply shader to all materials (per-instance)
    void applyShader(Shader shader);

    // Get bounding box (calculated from meshes)
    BoundingBox getBoundingBox() const;

    // Get the model path
    const std::string& getPath() const { return m_path; }

    // Reset current pose to bind pose
    void resetPose();

    // Animation support - prepares instance for animation playback
    void prepareForAnimation();

    // Swap per-instance animation data into meshes (call before UpdateModelAnimation)
    void bindAnimationData();

    // Restore shared animation data (call after drawing if needed)
    void unbindAnimationData();

    // Check if animation data is allocated
    bool hasAnimationData() const { return m_localMeshes != nullptr; }

private:
    friend class ModelManager;

    // Only ModelManager can create valid instances
    ModelInstance(ModelManager* manager, const std::string& path,
                  Mesh* meshes, int meshCount,
                  int* meshMaterial, Material* sourceMaterials, int materialCount,
                  BoneInfo* bones, int boneCount, Transform* bindPose);

    void release();
    void releaseAnimationData();

    ModelManager* m_manager = nullptr;
    std::string m_path;

    // Shared mesh data (owned by ModelManager)
    Mesh* m_sharedMeshes = nullptr;
    int m_meshCount = 0;
    int* m_meshMaterial = nullptr;

    // Per-instance mesh structures (for animation - pointers to shared vertex data but own animation buffers)
    Mesh* m_localMeshes = nullptr;

    // Shared bone data for animations (owned by ModelManager)
    BoneInfo* m_bones = nullptr;
    int m_boneCount = 0;
    Transform* m_bindPose = nullptr;

    // Per-instance current pose (owned by this instance)
    Transform* m_currentPose = nullptr;

    // Per-instance materials (owned by this instance)
    Material* m_materials = nullptr;
    int m_materialCount = 0;

    // Per-instance animation data for each mesh (owned by this instance)
    std::vector<MeshAnimationData> m_animData;
};

// ModelManager - caches models, shares mesh data, provides instances
class ModelManager {
public:
    ModelManager();
    ~ModelManager();

    // Acquire a model instance (loads if not cached, increments ref count)
    ModelInstance acquire(const std::string& path);

    // Preload models without creating instances
    void preload(const std::string& path);

    // Get cache stats
    int getCachedModelCount() const { return static_cast<int>(m_cache.size()); }
    int getRefCount(const std::string& path) const;

    // Unload all cached models
    void unloadAll();

    // GUI for debugging
    void gui();

private:
    friend class ModelInstance;

    // Called by ModelInstance destructor
    void release(const std::string& path);

    struct CachedModel {
        Model model;
        int refCount;
    };

    std::unordered_map<std::string, CachedModel> m_cache;
};

} // namespace moiras
