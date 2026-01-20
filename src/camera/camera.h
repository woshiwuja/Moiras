#include "../game/game.h"
#include <iostream>
#include <raylib.h>
namespace moiras {
class GameCamera : public GameObject {
private:
  Camera rcamera;
  int updateMode;

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
    rcamera.position = this->position; // Camera position
    rcamera.target = this->target;
    rcamera.up = this->up;
    rcamera.fovy = this->fovy;
    rcamera.projection = this->projection;
  }

  GameCamera(const std::string &name) : GameObject(name) {
    rcamera = {0};
    mode = CAMERA_FREE;
    rcamera.position = (Vector3){0.0f, 2.0f, 4.0f}; // Camera position
    rcamera.target = (Vector3){0.0f, 2.0f, 0.0f};   // Camera looking at point
    rcamera.up = (Vector3){0.0f, 1.0f,
                           0.0f}; // Camera up vector (rotation towards target)
    rcamera.fovy = 60.0f;         // Camera field-of-view Y
    rcamera.projection = CAMERA_PERSPECTIVE; // Camera projection type
  }
  GameCamera() {
    rcamera = {0};
    mode = CAMERA_FREE;
    rcamera.position = (Vector3){0.0f, 2.0f, 4.0f}; // Camera position
    rcamera.target = (Vector3){0.0f, 2.0f, 0.0f};   // Camera looking at point
    rcamera.up = (Vector3){0.0f, 1.0f,
                           0.0f}; // Camera up vector (rotation towards target)
    rcamera.fovy = 60.0f;         // Camera field-of-view Y
    rcamera.projection = CAMERA_PERSPECTIVE; // Camera projection type
  }
  void update() override {
    if (updateMode == 0) {
      this->mode = mode;
      UpdateCamera(&rcamera, mode);
    } else {
      UpdateCameraPro(&rcamera,
                      (Vector3){
                          (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) *
                                  GetFrameTime() * 5 - // Move forward-backward
                              (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) *
                                  GetFrameTime() * 5,
                          (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) *
                                  GetFrameTime() * 5 - // Move right-left
                              (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) *
                                  GetFrameTime() * 5,
                          0.0f // Move up-down
                      },
                      (Vector3){
                          GetMouseDelta().x * 0.05f, // Rotation: yaw
                          GetMouseDelta().y * 0.05f, // Rotation: pitch
                          0.0f                       // Rotation: roll
                      },
                      GetMouseWheelMove() * 2.0f); // Move to target (zoom)
    }
  }
  void beginMode3D() { BeginMode3D(rcamera); }
  void endMode3D() { EndMode3D(); }
  void beginDrawing() {
    BeginDrawing();
    ClearBackground(BLACK);
  }
  void endDrawing() { EndDrawing(); }
};
} // namespace moiras
