#pragma once
#include "../../rlImGui/rlImGui.h"
#include "../window/window.h"
#include "game_object.h"
#include <memory>
#include <raylib.h>
namespace moiras {
class Game {
  RenderTexture2D renderTarget;
  Shader outlineShader;

public:
  GameObject root;
  Game();
  ~Game() {

    rlImGuiShutdown(); // cleans up ImGui
    UnloadShader(outlineShader);
    UnloadRenderTexture(renderTarget);
  };
  void setup();
  void loop(Window window);
};
} // namespace moiras
