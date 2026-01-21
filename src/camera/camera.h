#include "../game/game_object.h"
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

namespace moiras {
class GameCamera : public GameObject {
private:
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

  void setUpdateMode(int mode) { updateMode = mode; }

  void update() override {
    if (updateMode == 0) {
      UpdateCamera(&rcamera, mode);
    } else {
      Vector2 mouseDelta = GetMouseDelta();
      if (fabsf(mouseDelta.x) > 300.0f)
        mouseDelta.x = 300.0f;
      if (fabsf(mouseDelta.y) > 300.0f)
        mouseDelta.y = 300.0f;
      UpdateCameraPro(
          &rcamera,
          (Vector3){
              (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) * GetFrameTime() * 5 -
                  (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) * GetFrameTime() *
                      5,
              (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) * GetFrameTime() * 5 -
                  (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) * GetFrameTime() *
                      5,
              0.0f},
          (Vector3){mouseDelta.x * 0.05f, mouseDelta.y * 0.05f, 0.0f},
          GetMouseWheelMove() * -2.0f);
    }
  }

  void beginMode3D() { BeginMode3D(rcamera); }

  void endMode3D() { EndMode3D(); }
  void beginDrawing() {
    BeginDrawing();
    ClearBackground(BLACK);
		rlClearScreenBuffers();
  }

  void endDrawing() { EndDrawing(); }
};
} // namespace moiras
