#include "structure.h"
#include "imgui.h"
#include <raymath.h>

namespace moiras {

Shader Structure::sharedShader = {0};

Structure::Structure()
    : GameObject("Structure"),
      model({0}),
      eulerRot({0.0f, 0.0f, 0.0f}),
      scale(1.0f),
      isPlaced(false),
      bounds({0}) {}

Structure::~Structure() {
    unloadModel();
}

Structure::Structure(Structure&& other) noexcept
    : GameObject(std::move(other)),
      model(other.model),
      eulerRot(other.eulerRot),
      scale(other.scale),
      isPlaced(other.isPlaced),
      modelPath(std::move(other.modelPath)),
      bounds(other.bounds) {
    other.model = {0};
}

Structure& Structure::operator=(Structure&& other) noexcept {
    if (this != &other) {
        unloadModel();
        GameObject::operator=(std::move(other));
        model = other.model;
        eulerRot = other.eulerRot;
        scale = other.scale;
        isPlaced = other.isPlaced;
        modelPath = std::move(other.modelPath);
        bounds = other.bounds;
        other.model = {0};
    }
    return *this;
}

void Structure::update() {
    GameObject::update();
}

void Structure::draw() {
    if (!isVisible || model.meshCount == 0) return;

    // Calcola la matrice di trasformazione
    Matrix matScale = MatrixScale(scale, scale, scale);
    Matrix matRotation = MatrixRotateXYZ(eulerRot);
    Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);

    model.transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

    DrawModel(model, {0, 0, 0}, 1.0f, WHITE);

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

void Structure::loadModel(const std::string& path) {
    unloadModel();
    modelPath = path;
    model = LoadModel(path.c_str());

    if (model.meshCount > 0) {
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
    if (model.meshCount > 0) {
        UnloadModel(model);
        model = {0};
    }
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
    if (model.meshCount > 0) {
        bounds = GetModelBoundingBox(model);

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
    for (int i = 0; i < model.materialCount; i++) {
        model.materials[i].shader = shader;
    }
}

void Structure::setSharedShader(Shader shader) {
    sharedShader = shader;
}

} // namespace moiras
