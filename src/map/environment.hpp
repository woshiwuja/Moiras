#pragma once
#include "../game/game_object.h"
#include <raylib.h>
#include <vector>
#include <string>

namespace moiras {

enum class RockMeshType {
    CUBE = 0,
    SPHERE,
    HEMISPHERE,
    CYLINDER,
    CONE,
    COUNT
};

struct RockPatch {
    Mesh mesh;
    Material material;
    RockMeshType meshType;
    std::vector<Matrix> transforms;

    RockPatch() : mesh{0}, material{0}, meshType(RockMeshType::CUBE) {}
};

class EnvironmentalObject : public GameObject {
private:
    std::vector<RockPatch> m_patches;
    Shader m_instancingShader;
    const Model *m_terrain;
    float m_rockSize;
    float m_spawnRadius;
    bool m_initialized;
    bool m_shaderLoaded;

    // Camera culling
    Vector3 m_cameraPos;
    float m_cullDistance;
    std::vector<Matrix> m_visibleBuffer; // temp buffer per draw

    // Brush
    bool m_brushMode;
    float m_brushRadius;
    int m_brushDensity;
    int m_activePatch;

    Mesh generateMesh(RockMeshType type, float size);
    void loadShader();
    int findOrCreatePatch(RockMeshType type);

public:
    EnvironmentalObject(float rockSize = 1.0f, float spawnRadius = 200.0f);
    ~EnvironmentalObject();
    EnvironmentalObject(const EnvironmentalObject &) = delete;
    EnvironmentalObject &operator=(const EnvironmentalObject &) = delete;

    void generate(const Model &terrain, int count, RockMeshType type);
    void updateCameraPos(Vector3 camPos) { m_cameraPos = camPos; }

    // Brush
    void paintAt(Vector3 center);
    void eraseAt(Vector3 center);
    void clearAll();
    bool isBrushMode() const { return m_brushMode; }
    void setBrushMode(bool enabled) { m_brushMode = enabled; }
    float getBrushRadius() const { return m_brushRadius; }
    void setBrushRadius(float r) { m_brushRadius = r; }
    int getBrushDensity() const { return m_brushDensity; }
    void setBrushDensity(int d) { m_brushDensity = d; }
    float getCullDistance() const { return m_cullDistance; }
    void setCullDistance(float d) { m_cullDistance = d; }

    // Patches
    int getActivePatch() const { return m_activePatch; }
    void setActivePatch(int idx);
    int getPatchCount() const { return (int)m_patches.size(); }
    const RockPatch &getPatch(int idx) const { return m_patches[idx]; }
    RockMeshType getActiveMeshType() const;
    void setActiveMeshType(RockMeshType type);
    int addPatch(RockMeshType type);

    void draw() override;
    void gui() override;

    int getTotalInstanceCount() const;
};

const char *rockMeshTypeName(RockMeshType type);

} // namespace moiras
