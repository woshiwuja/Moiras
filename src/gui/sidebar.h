// filepicker.h
#include "../game/game_object.h"
#include "imgui.h"

namespace moiras {

class Sidebar : public GameObject {
public:
  Sidebar();
  ~Sidebar() = default;

  void update() override;
  void gui() override;

  void drawDebugWindows() {
    if (showImGuiDemo) {
      ImGui::ShowDemoWindow(&showImGuiDemo);
    }
    if (showStyleEditor) {
      ImGui::Begin("Style Editor", &showStyleEditor);
      ImGui::ShowStyleEditor();
      ImGui::End();
    }
  }

private:
  void drawSceneTab(GameObject *obj);
  void drawGameObjectTree(GameObject *obj);

  float sidebarWidth;
  bool showImGuiDemo = false;
  bool showStyleEditor = false;

  void drawSettingsTab();
  int currentTab;
};

} // namespace moiras
