#include "character.h"
#include <limits>
#include <raylib.h>
#include <raymath.h>

using moiras::Character;

Character::Character()
    : health(100), scale(1.0f), name("Character"), isVisible(true),
      quat_rotation{1.0, 0.0, 0.0, 0.0}, eulerRot{0, 0, 0} {
  model = {0}; // Inizializza a zero
  quat_rotation = QuaternionFromEuler(eulerRot.x, eulerRot.y, eulerRot.z);
}

Character::Character(Model *modelPtr)
    : health(100), scale(1.0f), name("Character"), isVisible(true),
      quat_rotation{1.0, 0.0, 0.0, 0.0}, eulerRot{0, 0, 0} {
  if (modelPtr != nullptr) {
    model = *modelPtr;
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

// Move constructor
Character::Character(Character&& other) noexcept
    : GameObject(std::move(other)),
      health(other.health),
      name(std::move(other.name)),
      eulerRot(other.eulerRot),
      isVisible(other.isVisible),
      scale(other.scale),
      model(other.model),
      quat_rotation(other.quat_rotation) {
  // Resetta il modello dell'altro per evitare double-free
  other.model = {0};
  TraceLog(LOG_INFO, "Character moved: %s", name.c_str());
}

// Move assignment
Character& Character::operator=(Character&& other) noexcept {
  if (this != &other) {
    GameObject::operator=(std::move(other));
    
    // Libera il modello corrente
    if (model.meshCount > 0) {
      UnloadModel(model);
    }
    
    // Trasferisci i dati
    health = other.health;
    name = std::move(other.name);
    eulerRot = other.eulerRot;
    isVisible = other.isVisible;
    scale = other.scale;
    model = other.model;
    quat_rotation = other.quat_rotation;
    
    // Resetta il modello dell'altro
    other.model = {0};
    
    TraceLog(LOG_INFO, "Character move-assigned: %s", name.c_str());
  }
  return *this;
}

void Character::loadModel(const std::string &path) {
  TraceLog(LOG_INFO, "Loading model: %s", path.c_str());
  
  // Scarica il modello precedente se esiste
  if (model.meshCount > 0) {
    TraceLog(LOG_INFO, "Unloading previous model");
    UnloadModel(model);
    model = {0};
  }
  
  model = LoadModel(path.c_str());
  
  if (model.meshCount > 0) {
    TraceLog(LOG_INFO, "Model loaded successfully: %d meshes", model.meshCount);
  } else {
    TraceLog(LOG_ERROR, "Failed to load model or model has no meshes");
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
        
        character->loadModel(droppedFiles.paths[0]);
        
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
  Quaternion deltaQuat =
      QuaternionFromAxisAngle(axis, angularSpeed * GetFrameTime());
  quat_rotation = (QuaternionMultiply(deltaQuat, quat_rotation));
  quat_rotation = QuaternionNormalize(quat_rotation);
  handleDroppedModel();
}

void Character::draw() {
  if (isVisible) {
    if (model.meshCount > 0) {
      DrawModel(model, position, scale, WHITE);
    } else {
      DrawCube(position, scale, scale, scale, GREEN);
      DrawCubeWires(Vector3{position.x, position.y + (scale / 2), position.z},
                    scale, scale, scale, BLACK);
    }
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

void Character::gui() {
  DrawText(TextFormat("Name: %s", name.c_str()), 900, 300, 24, BLUE);
  GuiSlider(Rectangle{100, 300, 200, 60}, "-", "+", &scale, 0.1, 100);
}
