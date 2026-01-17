#include "character/character.h"
#include "raylib.h"
#include "rcamera.h"

#define MAX_COLUMNS 20

int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "Moiras");
  Camera camera = {0};
  camera.position = (Vector3){0.0f, 2.0f, 4.0f}; // Camera position
  camera.target = (Vector3){0.0f, 2.0f, 0.0f};   // Camera looking at point
  camera.up =
      (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 60.0f;             // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera projection type

  int cameraMode = CAMERA_FIRST_PERSON;
  DisableCursor(); // Limit cursor to relative movement inside the window
  Character mainChar = Character();
  SetTargetFPS(144); // Set our game to run at 60 frames-per-second
  SetExitKey(KEY_END);
  ToggleFullscreen();
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    UpdateCameraPro(
        &camera,
        (Vector3){
            (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) * GetFrameTime() *
                    5 - // Move forward-backward
                (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) * GetFrameTime() * 5,
            (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) * GetFrameTime() *
                    5 - // Move right-left
                (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) * GetFrameTime() * 5,
            0.0f // Move up-down
        },
        (Vector3){
            GetMouseDelta().x * 0.05f, // Rotation: yaw
            GetMouseDelta().y * 0.05f, // Rotation: pitch
            0.0f                       // Rotation: roll
        },
        GetMouseWheelMove() * 2.0f); // Move to target (zoom)
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){600.0f, 600.0f},
              LIGHTGRAY); // Draw ground
    DrawCube((Vector3){-16.0f, 2.5f, 0.0f}, 1.0f, 5.0f, 32.0f,
             BLUE); // Draw a blue wall
    DrawCube((Vector3){16.0f, 2.5f, 0.0f}, 1.0f, 5.0f, 32.0f,
             LIME); // Draw a green wall
    DrawCube((Vector3){0.0f, 2.5f, 16.0f}, 32.0f, 5.0f, 1.0f,
             GOLD); // Draw a yellow wall

    EndMode3D();
    DrawRectangle(5, 5, 330, 100, Fade(SKYBLUE, 0.5f));
    DrawRectangleLines(5, 5, 330, 100, BLUE);
    DrawText("Camera controls:", 15, 15, 10, BLACK);
    DrawText("- Move keys: W, A, S, D, Space, Left-Ctrl", 15, 30, 10, BLACK);
    DrawText("- Look around: arrow keys or mouse", 15, 45, 10, BLACK);
    DrawText("- Camera mode keys: 1, 2, 3, 4", 15, 60, 10, BLACK);
    DrawText("- Zoom keys: num-plus, num-minus or mouse scroll", 15, 75, 10,
             BLACK);
    DrawText("- Camera projection key: P", 15, 90, 10, BLACK);

    DrawRectangle(600, 5, 195, 100, Fade(SKYBLUE, 0.5f));
    DrawRectangleLines(600, 5, 195, 100, BLUE);

    DrawText("Camera status:", 610, 15, 10, BLACK);
    DrawText(TextFormat("- Mode: %s",
                        (cameraMode == CAMERA_FREE)           ? "FREE"
                        : (cameraMode == CAMERA_FIRST_PERSON) ? "FIRST_PERSON"
                        : (cameraMode == CAMERA_THIRD_PERSON) ? "THIRD_PERSON"
                        : (cameraMode == CAMERA_ORBITAL)      ? "ORBITAL"
                                                              : "CUSTOM"),
             610, 30, 10, BLACK);
    DrawText(
        TextFormat("- Projection: %s",
                   (camera.projection == CAMERA_PERSPECTIVE)    ? "PERSPECTIVE"
                   : (camera.projection == CAMERA_ORTHOGRAPHIC) ? "ORTHOGRAPHIC"
                                                                : "CUSTOM"),
        610, 45, 10, BLACK);
    DrawText(TextFormat("- Position: (%06.3f, %06.3f, %06.3f)",
                        camera.position.x, camera.position.y,
                        camera.position.z),
             610, 60, 10, BLACK);
    DrawText(TextFormat("- Target: (%06.3f, %06.3f, %06.3f)", camera.target.x,
                        camera.target.y, camera.target.z),
             610, 75, 10, BLACK);
    DrawText(TextFormat("- Up: (%06.3f, %06.3f, %06.3f)", camera.up.x,
                        camera.up.y, camera.up.z),
             610, 90, 10, BLACK);

    EndDrawing();
  }
  CloseWindow(); // Close window and OpenGL context
  return 0;
}
