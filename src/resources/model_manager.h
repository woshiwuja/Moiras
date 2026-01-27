#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>

namespace moiras {

class ModelManager;

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
    bool isValid() const { return m_meshes != nullptr && m_meshCount > 0; }

    // Access shared mesh data (read-only)
    Mesh* meshes() const { return m_meshes; }
    int meshCount() const { return m_meshCount; }
    int* meshMaterial() const { return m_meshMaterial; }

    // Access bone data for animations
    BoneInfo* bones() const { return m_bones; }
    int boneCount() const { return m_boneCount; }
    Transform* bindPose() const { return m_bindPose; }

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

private:
    friend class ModelManager;

    // Only ModelManager can create valid instances
    ModelInstance(ModelManager* manager, const std::string& path,
                  Mesh* meshes, int meshCount,
                  int* meshMaterial, Material* sourceMaterials, int materialCount,
                  BoneInfo* bones, int boneCount, Transform* bindPose);

    void release();

    ModelManager* m_manager = nullptr;
    std::string m_path;

    // Shared mesh data (owned by ModelManager)
    Mesh* m_meshes = nullptr;
    int m_meshCount = 0;
    int* m_meshMaterial = nullptr;

    // Shared bone data for animations (owned by ModelManager)
    BoneInfo* m_bones = nullptr;
    int m_boneCount = 0;
    Transform* m_bindPose = nullptr;

    // Per-instance materials (owned by this instance)
    Material* m_materials = nullptr;
    int m_materialCount = 0;
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
