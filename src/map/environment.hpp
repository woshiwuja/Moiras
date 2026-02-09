#pragma once
#include "../game/game_object.h"
#include <raylib.h>
#include <vector>

namespace moiras {

enum class RockMeshType {
    CUBE = 0,
    SPHERE,
    HEMISPHERE,
    CYLINDER,
    CONE,
    COUNT
};

class EnvironmentalObject : public GameObject {
private:
    Mesh m_rockMesh;
    Material m_material;
    Shader m_currentShader;
    std::vector<Matrix> m_transforms;
    const Model *m_terrain;
    int m_instanceCount;
    int m_targetInstanceCount;
    float m_rockSize;
    float m_spawnRadius;
    bool m_initialized;
    bool m_hasShader;
    RockMeshType m_meshType;

    // Brush
    bool m_brushMode;
    float m_brushRadius;
    int m_brushDensity;

    Mesh generateMesh(RockMeshType type, float size);

public:
    EnvironmentalObject(int instanceCount = 500, float rockSize = 1.0f, float spawnRadius = 200.0f);
    ~EnvironmentalObject();
    EnvironmentalObject(const EnvironmentalObject &) = delete;
    EnvironmentalObject &operator=(const EnvironmentalObject &) = delete;

    void generate(const Model &terrain);
    void setShader(Shader shader);
    void setMeshType(RockMeshType type);
    RockMeshType getMeshType() const { return m_meshType; }

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

    void draw() override;
    void gui() override;

    int getInstanceCount() const { return m_instanceCount; }
    Mesh &getMesh() { return m_rockMesh; }
    const std::vector<Matrix> &getTransforms() const { return m_transforms; }
};

const char *rockMeshTypeName(RockMeshType type);

} // namespace moiras
