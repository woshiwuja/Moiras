#pragma once
#include "../game/game_object.h"
#include <iostream>
#include <limits>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

namespace moiras {
void handleCursor();
class GameCamera : public GameObject {
private:
  Ray ray;
  Camera rcamera;
  int updateMode = 1; // AGGIUNTO: inizializzato a 1 per UpdateCameraPro

public:
  int mode;
  Vector3 position;
  Vector3 target;
  Vector3 up;
  float fovy;
  int projection;

  GameCamera(Vector3 position, Vector3 target, Vector3 up, float fovy,
             int projection, int mode) {
    rcamera = {0};
    this->mode = mode;
    this->position = position;
    this->target = target;
    this->up = up;
    this->fovy = fovy;
    this->projection = projection;
    rcamera.position = this->position;
    rcamera.target = this->target;
    rcamera.up = this->up;
    rcamera.fovy = this->fovy;
    rcamera.projection = this->projection;
    updateMode = 1;
  }

  GameCamera(const std::string &name) : GameObject(name) {
    rcamera = {0};
    mode = CAMERA_FREE;
    rcamera.position = (Vector3){0.0f, 2.0f, 4.0f};
    rcamera.target = (Vector3){0.0f, 2.0f, 0.0f};
    rcamera.up = (Vector3){0.0f, 1.0f, 0.0f};
    rcamera.fovy = 45.0f;
    rcamera.projection = CAMERA_PERSPECTIVE;
    updateMode = 1;
  }

  GameCamera() {
    rcamera = {0};
    mode = CAMERA_FREE;
    rcamera.position = (Vector3){0.0f, 2.0f, 4.0f};
    rcamera.target = (Vector3){0.0f, 2.0f, 0.0f};
    rcamera.up = (Vector3){0.0f, 1.0f, 0.0f};
    rcamera.fovy = 60.0f;
    rcamera.projection = CAMERA_PERSPECTIVE;
    updateMode = 1;
  }

  void setUpdateMode(int mode) { updateMode = mode; };

  void cameraControl();

  void update() override;

  void beginMode3D() { BeginMode3D(rcamera); }

  void endMode3D() { EndMode3D(); }
  void beginDrawing() {
    BeginDrawing();
    ClearBackground(BLACK);
    rlClearScreenBuffers();
  }

  void endDrawing() { EndDrawing(); }
  Ray getRay() { return ray; };
};

} // namespace moiras
