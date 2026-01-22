#include "character.h"
#include <raylib.h>
#include <raymath.h>
using moiras::Character;

Character::Character()
    : health(100), scale(1.0f), model(nullptr),
      quat_rotation{1.0, 0.0, 0.0, 0.0} {
  quat_rotation = QuaternionFromEuler(eulerRot.x, eulerRot.y, eulerRot.z);
}

Character::Character(Model *model)
    : health(100), scale(1.0f), model(model),
      quat_rotation{1.0, 0.0, 0.0, 0.0} {
  quat_rotation = QuaternionFromEuler(eulerRot.x, eulerRot.y, eulerRot.z);
}

void Character::update() {
  Vector3 axis = {0, 1, 0};
  float angularSpeed = 0.0f;
  Quaternion deltaQuat =
      QuaternionFromAxisAngle(axis, angularSpeed * GetFrameTime());
  quat_rotation = (QuaternionMultiply(deltaQuat, quat_rotation));
  quat_rotation = QuaternionNormalize(quat_rotation);
}

void Character::draw() {
  if (isVisible) {
    if (model != nullptr) {
      DrawModel(*model, Vector3{10, 10, 10}, scale, WHITE);
    } else {
      DrawCube(Vector3{position.x, position.y + (scale / 2), position.z}, scale,
               scale, scale, GREEN);
      DrawCubeWires(Vector3{position.x, position.y + (scale / 2), position.z},
                    scale, scale, scale, BLACK);
    }
  }
}

void Character::gui() {
  GuiLabel(Rectangle{100, 300, 180, 20}, TextFormat("[%s]", name.c_str()));
  GuiSlider(Rectangle{100, 300, 200, 60}, "-", "+", &scale, 0.1, 100);
}
