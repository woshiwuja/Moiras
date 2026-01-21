#pragma once
#include "../window/window.h"
#include "game_object.h"
#include <memory>
#include <raylib.h>
namespace moiras {
class Game {
public:
  GameObject root;
  Game();
  void loop(Window window);
};
} // namespace moiras
