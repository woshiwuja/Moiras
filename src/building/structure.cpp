#include "structure.h"
#include "imgui.h"
#include <raymath.h>

namespace moiras {

Shader Structure::sharedShader = {0};

Structure::Structure()
    : GameObject("Structure"),
      eulerRot({0.0f, 0.0f, 0.0f}),
      scale(1.0f),
      isPlaced(false),
      bounds({0}) {}

Structure::~Structure() {
    // ModelInstance destructor handles cleanup automatically
}

Structure::Structure(Structure&& other) noexcept
    : GameObject(std::move(other)),
      modelInstance(std::move(other.modelInstance)),
      eulerRot(other.eulerRot),
      scale(other.scale),
      isPlaced(other.isPlaced),
      modelPath(std::move(other.modelPath)),
      bounds(other.bounds) {
}

Structure& Structure::operator=(Structure&& other) noexcept {
    if (this != &other) {
        GameObject::operator=(std::move(other));
        modelInstance = std::move(other.modelInstance);
        eulerRot = other.eulerRot;
        scale = other.scale;
        isPlaced = other.isPlaced;
        modelPath = std::move(other.modelPath);
        bounds = other.bounds;
    }
    return *this;
}

void Structure::update() {
    GameObject::update();
}

void Structure::draw() {
    if (!isVisible || !modelInstance.isValid()) return;

    // Calcola la matrice di trasformazione
    Matrix matScale = MatrixScale(scale, scale, scale);
    Matrix matRotation = MatrixRotateXYZ(eulerRot);
    Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);

    Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

    // Draw each mesh with its material
    for (int i = 0; i < modelInstance.meshCount(); i++) {
        int materialIndex = modelInstance.meshMaterial()[i];
        Material material = modelInstance.materials()[materialIndex];
        DrawMesh(modelInstance.meshes()[i], material, transform);
    }

    GameObject::draw();
}

void Structure::gui() {
    if (ImGui::TreeNode(name.c_str())) {
        ImGui::DragFloat3("Position", &position.x, 0.1f);
        ImGui::DragFloat3("Rotation", &eulerRot.x, 0.01f, -PI, PI);
        ImGui::DragFloat("Scale", &scale, 0.01f, 0.01f, 10.0f);
        ImGui::Checkbox("Visible", &isVisible);
        ImGui::Text("Model: %s", modelPath.c_str());
        ImGui::TreePop();
    }

    GameObject::gui();
}

void Structure::loadModel(ModelManager& manager, const std::string& path) {
    unloadModel();
    modelPath = path;
    modelInstance = manager.acquire(path);

    if (modelInstance.isValid()) {
        // Applica shader condiviso se disponibile
        if (sharedShader.id != 0) {
            applyShader(sharedShader);
        }
        updateBounds();
        TraceLog(LOG_INFO, "Structure loaded: %s", path.c_str());
    } else {
        TraceLog(LOG_WARNING, "Failed to load structure: %s", path.c_str());
    }
}

void Structure::unloadModel() {
    modelInstance = ModelInstance(); // Release via move assignment
}

void Structure::snapToGround(const Model& ground) {
    // Raycast verso il basso per trovare il terreno
    Ray ray;
    ray.position = {position.x, position.y + 1000.0f, position.z};
    ray.direction = {0.0f, -1.0f, 0.0f};

    RayCollision collision = {0};
    collision.hit = false;
    collision.distance = FLT_MAX;

    for (int m = 0; m < ground.meshCount; m++) {
        RayCollision meshHitInfo = GetRayCollisionMesh(ray, ground.meshes[m], ground.transform);
        if (meshHitInfo.hit && meshHitInfo.distance < collision.distance) {
            collision = meshHitInfo;
        }
    }

    if (collision.hit) {
        position.y = collision.point.y;
        updateBounds();
    }
}

void Structure::updateBounds() {
    if (modelInstance.isValid()) {
        bounds = modelInstance.getBoundingBox();

        // Trasforma i bounds in base alla posizione e scala
        Vector3 size = {
            (bounds.max.x - bounds.min.x) * scale,
            (bounds.max.y - bounds.min.y) * scale,
            (bounds.max.z - bounds.min.z) * scale
        };

        bounds.min = {
            position.x - size.x * 0.5f,
            position.y,
            position.z - size.z * 0.5f
        };
        bounds.max = {
            position.x + size.x * 0.5f,
            position.y + size.y,
            position.z + size.z * 0.5f
        };
    }
}

void Structure::applyShader(Shader shader) {
    if (modelInstance.isValid()) {
        modelInstance.applyShader(shader);
    }
}

void Structure::setSharedShader(Shader shader) {
    sharedShader = shader;
}

} // namespace moiras
