#pragma once
#include "../window/window.h"
#include "game_object.h"
#include <memory>
#include <raylib.h>
#include "../../rlImGui/rlImGui.h"
namespace moiras {
class Game {
public:
  GameObject root;
  Game();
  ~Game() {

    rlImGuiShutdown(); // cleans up ImGui
  };
  void setup();
  void loop(Window window);
};
} // namespace moiras
