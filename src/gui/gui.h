#pragma once

#include "../game/game_object.h"
#include "imgui.h"
namespace moiras {
class Gui : public GameObject {
private:
  ImGuiIO io;

public:
  ~Gui();
  Gui();
};
} // namespace moiras
