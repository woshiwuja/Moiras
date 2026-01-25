#include "character.h"
#include <limits>
#include <raylib.h>
#include <raymath.h>

namespace moiras {

// Definizione dello shader statico
Shader Character::sharedShader = {0};

void Character::setSharedShader(Shader shader) {
    sharedShader = shader;
    TraceLog(LOG_INFO, "Character shared shader set, ID: %d", shader.id);
}

void Character::applyShader(Shader shader) {
    if (model.meshCount > 0 && shader.id > 0) {
        for (int i = 0; i < model.materialCount; i++) {
            model.materials[i].shader = shader;
        }
        TraceLog(LOG_INFO, "Applied shader ID %d to character '%s' (%d materials)", 
                 shader.id, name.c_str(), model.materialCount);
    }
}

Character::Character()
    : health(100), scale(1.0f), name("Character"), isVisible(true),
      quat_rotation{1.0, 0.0, 0.0, 0.0}, eulerRot{0, 0, 0} {
    model = {0};
    quat_rotation = QuaternionFromEuler(eulerRot.x, eulerRot.y, eulerRot.z);
}

Character::Character(Model *modelPtr)
    : health(100), scale(1.0f), name("Character"), isVisible(true),
      quat_rotation{1.0, 0.0, 0.0, 0.0}, eulerRot{0, 0, 0} {
    if (modelPtr != nullptr) {
        model = *modelPtr;
        // Applica lo shader condiviso se disponibile
        if (sharedShader.id > 0) {
            applyShader(sharedShader);
        }
    } else {
        model = {0};
    }
    quat_rotation = QuaternionFromEuler(eulerRot.x, eulerRot.y, eulerRot.z);
}

Character::~Character() {
    TraceLog(LOG_INFO, "Destroying character: %s", name.c_str());
    if (model.meshCount > 0) {
        UnloadModel(model);
        model = {0};
    }
}

Character::Character(Character &&other) noexcept
    : GameObject(std::move(other)), health(other.health),
      name(std::move(other.name)), eulerRot(other.eulerRot),
      isVisible(other.isVisible), scale(other.scale), model(other.model),
      quat_rotation(other.quat_rotation) {
    other.model = {0};
    TraceLog(LOG_INFO, "Character moved: %s", name.c_str());
}

Character &Character::operator=(Character &&other) noexcept {
    if (this != &other) {
        GameObject::operator=(std::move(other));

        if (model.meshCount > 0) {
            UnloadModel(model);
        }

        health = other.health;
        name = std::move(other.name);
        eulerRot = other.eulerRot;
        isVisible = other.isVisible;
        scale = other.scale;
        model = other.model;
        quat_rotation = other.quat_rotation;

        other.model = {0};

        TraceLog(LOG_INFO, "Character move-assigned: %s", name.c_str());
    }
    return *this;
}

void Character::loadModel(const std::string &path) {
    TraceLog(LOG_INFO, "Loading model: %s", path.c_str());

    if (model.meshCount > 0) {
        UnloadModel(model);
        model = {0};
    }

    model = LoadModel(path.c_str());

    if (model.meshCount > 0) {
        TraceLog(LOG_INFO, "Model loaded: %d meshes, %d materials", 
                 model.meshCount, model.materialCount);
        
        // Debug: stampa info materiali
        for (int i = 0; i < model.materialCount; i++) {
            Material& mat = model.materials[i];
            TraceLog(LOG_INFO, "Material %d:", i);
            TraceLog(LOG_INFO, "  - Albedo texture ID: %d", mat.maps[MATERIAL_MAP_ALBEDO].texture.id);
            TraceLog(LOG_INFO, "  - Normal texture ID: %d", mat.maps[MATERIAL_MAP_NORMAL].texture.id);
            TraceLog(LOG_INFO, "  - Metalness texture ID: %d", mat.maps[MATERIAL_MAP_METALNESS].texture.id);
            TraceLog(LOG_INFO, "  - Albedo color: %d,%d,%d,%d", 
                     mat.maps[MATERIAL_MAP_ALBEDO].color.r,
                     mat.maps[MATERIAL_MAP_ALBEDO].color.g,
                     mat.maps[MATERIAL_MAP_ALBEDO].color.b,
                     mat.maps[MATERIAL_MAP_ALBEDO].color.a);
            TraceLog(LOG_INFO, "  - Metalness value: %.2f", mat.maps[MATERIAL_MAP_METALNESS].value);
            TraceLog(LOG_INFO, "  - Roughness value: %.2f", mat.maps[MATERIAL_MAP_ROUGHNESS].value);
        }
        
        // Applica lo shader
        if (sharedShader.id > 0) {
            applyShader(sharedShader);
        }
    } else {
        TraceLog(LOG_ERROR, "Failed to load model");
    }
}

void Character::unloadModel() {
    if (model.meshCount > 0) {
        UnloadModel(model);
        model = {0};
    }
}

void Character::handleDroppedModel() {
    if (IsFileDropped()) {
        FilePathList droppedFiles = LoadDroppedFiles();

        if (droppedFiles.count == 1) {
            if (IsFileExtension(droppedFiles.paths[0], ".obj") ||
                IsFileExtension(droppedFiles.paths[0], ".gltf") ||
                IsFileExtension(droppedFiles.paths[0], ".glb") ||
                IsFileExtension(droppedFiles.paths[0], ".vox") ||
                IsFileExtension(droppedFiles.paths[0], ".iqm") ||
                IsFileExtension(droppedFiles.paths[0], ".m3d")) {

                auto character = std::make_unique<Character>();
                character->name = "Dropped Model";
                character->loadModel(droppedFiles.paths[0]);  // Shader applicato automaticamente qui

                if (character->model.meshCount > 0) {
                    auto bounds = GetMeshBoundingBox(character->model.meshes[0]);
                    TraceLog(LOG_INFO, "Model bounds: %.2f, %.2f, %.2f",
                             bounds.max.x - bounds.min.x, 
                             bounds.max.y - bounds.min.y,
                             bounds.max.z - bounds.min.z);
                }

                auto parent = this->getParent();
                if (parent != nullptr) {
                    TraceLog(LOG_INFO, "Adding character to parent");
                    parent->addChild(std::move(character));
                    TraceLog(LOG_INFO, "Character added successfully");
                } else {
                    TraceLog(LOG_ERROR, "Parent is null!");
                }
            }
        }

        UnloadDroppedFiles(droppedFiles);
    }
}

void Character::update() {
    Vector3 axis = {0, 1, 0};
    float angularSpeed = 0.0f;
    Quaternion deltaQuat = QuaternionFromAxisAngle(axis, angularSpeed * GetFrameTime());
    quat_rotation = QuaternionMultiply(deltaQuat, quat_rotation);
    quat_rotation = QuaternionNormalize(quat_rotation);
    handleDroppedModel();
}

void Character::draw() {
    if (!isVisible) return;
    
    if (model.meshCount > 0) {
        Quaternion q = QuaternionFromEuler(
            eulerRot.x * DEG2RAD,
            eulerRot.y * DEG2RAD,
            eulerRot.z * DEG2RAD
        );
        
        Vector3 axis;
        float angle;
        QuaternionToAxisAngle(q, &axis, &angle);
        
        // Calcola la matrice di trasformazione
        Matrix matScale = MatrixScale(scale, scale, scale);
        Matrix matRotation = MatrixRotate(axis, angle * DEG2RAD);
        Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);
        
        Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
        
        // Disegna ogni mesh con il suo materiale
        for (int i = 0; i < model.meshCount; i++) {
            int materialIndex = model.meshMaterial[i];
            Material material = model.materials[materialIndex];
            
            // DrawMesh applica automaticamente lo shader del materiale
            DrawMesh(model.meshes[i], material, transform);
        }
    } else {
        DrawCube(position, scale, scale, scale, GREEN);
        DrawCubeWires(Vector3{position.x, position.y + (scale / 2), position.z},
                      scale, scale, scale, BLACK);
    }
}

void Character::snapToGround(const Model &ground) {
    Ray downRay;
    downRay.position = {position.x, position.y + 100.0f, position.z};
    downRay.direction = {0.0f, -1.0f, 0.0f};

    RayCollision closest = {0};
    closest.hit = false;
    closest.distance = std::numeric_limits<float>::max();

    for (int m = 0; m < ground.meshCount; m++) {
        RayCollision hit = GetRayCollisionMesh(downRay, ground.meshes[m], ground.transform);
        if (hit.hit && hit.distance < closest.distance) {
            closest = hit;
        }
    }

    if (closest.hit) {
        position.y = closest.point.y;
    }
}

void Character::gui() {}

} // namespace moiras
