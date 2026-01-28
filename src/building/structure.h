#pragma once

#include "../game/game_object.h"
#include "../resources/model_manager.h"
#include <raylib.h>
#include <string>

// Forward declaration for dtObstacleRef
typedef unsigned int dtObstacleRef;

namespace moiras {

// Rappresenta una struttura piazzata nel mondo
class Structure : public GameObject {
public:
    ModelInstance modelInstance;
    Vector3 eulerRot;
    Quaternion rotation;
    float scale;
    bool isPlaced;
    std::string modelPath;

    // Bounding box per collisioni e navmesh obstacles
    BoundingBox bounds;

    // NavMesh obstacle reference (0 = not registered)
    dtObstacleRef navMeshObstacleRef = 0;

    Structure();
    ~Structure();

    // Move semantics
    Structure(Structure&& other) noexcept;
    Structure& operator=(Structure&& other) noexcept;

    // No copy
    Structure(const Structure&) = delete;
    Structure& operator=(const Structure&) = delete;

    void update() override;
    void draw() override;
    void gui() override;

    void loadModel(ModelManager& manager, const std::string& path);
    void unloadModel();
    void snapToGround(const Model& ground);
    void updateBounds();

    // Applica lo shader a tutti i materiali
    void applyShader(Shader shader);

    // Shader condiviso
    static Shader sharedShader;
    static void setSharedShader(Shader shader);

    // Check if model is loaded
    bool hasModel() const { return modelInstance.isValid(); }
};

} // namespace moiras
