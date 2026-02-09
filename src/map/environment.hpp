#pragma once
#include "../game/game_object.h"
#include <raylib.h>
#include <vector>

namespace moiras {

class EnvironmentalObject : public GameObject {
private:
    Mesh m_rockMesh;
    Material m_material;
    std::vector<Matrix> m_transforms;
    int m_instanceCount;
    float m_rockSize;
    float m_spawnRadius;
    bool m_initialized;

public:
    EnvironmentalObject(int instanceCount = 500, float rockSize = 1.0f, float spawnRadius = 200.0f);
    ~EnvironmentalObject();
    EnvironmentalObject(const EnvironmentalObject &) = delete;
    EnvironmentalObject &operator=(const EnvironmentalObject &) = delete;

    void generate(const Model &terrain);
    void setShader(Shader shader);

    void draw() override;
    void gui() override;

    int getInstanceCount() const { return m_instanceCount; }
    Mesh &getMesh() { return m_rockMesh; }
    const std::vector<Matrix> &getTransforms() const { return m_transforms; }
};

} // namespace moiras
