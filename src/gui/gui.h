#pragma once

#include "../game/game_object.h"
#include "../resources/model_manager.h"
#include "imgui.h"
namespace moiras {
class Gui : public GameObject {
private:
  ImGuiIO io;

public:
  ~Gui();
  Gui();
  void setModelManager(ModelManager* manager);
};
} // namespace moiras
