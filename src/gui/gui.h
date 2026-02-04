#pragma once

#include "../game/game_object.h"
#include "../resources/model_manager.h"
#include "imgui.h"
#include <map>
namespace moiras {
class Gui : public GameObject {
private:
  ImGuiIO io;

public:
  // Map of font sizes to ImFont pointers
  static std::map<int, ImFont*> editorFonts;
  
  ~Gui();
  Gui();
  void setModelManager(ModelManager* manager);
  
  // Get font by size (returns closest available)
  static ImFont* getEditorFont(int size);
};
} // namespace moiras
