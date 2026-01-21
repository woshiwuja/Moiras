#include "raylib.h"
#include <cstdlib>
#include <string>
using std::string;
namespace moiras {

class Window {
public:
  int width;
  int height;
  string title;
  int targetFps;
  bool fullscreen;
  int exitKey;
  Window() {
    width = 800;
    height = 600;
    title = "default title";
    fullscreen = false;
    exitKey = KEY_ESCAPE;
    targetFps = 60;
  }
  ~Window() {
    CloseWindow(); // Close window and OpenGL context
  }
  Window(int width, int height, string title, int exitKey, int targetFps,
         bool fullscreen)
      : width(width), height(height), title(title), exitKey(exitKey),
        targetFps(targetFps) {}
  void init() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, title.c_str());
    if (fullscreen) {
      ToggleFullscreen();
    }
    SetExitKey(exitKey);
    SetTargetFPS(targetFps);
    DisableCursor(); // Limit cursor to relative movement inside the window
  }
  bool shouldClose() { return WindowShouldClose(); }
};

} // namespace moiras
