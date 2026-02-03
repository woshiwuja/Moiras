#include "gui.h"
#include "../lights/lightmanager.h"
#include "asset_spawner.h"
#include "sidebar.h"
#include "script_editor.h"
#include <imgui.h>
namespace moiras {

Gui::~Gui() {}

Gui::Gui() {
  auto sidebar = std::make_unique<Sidebar>();
  auto sidebarPtr = sidebar.get();
  addChild(std::move(sidebar));
  
  auto asset_spawner = std::make_unique<AssetSpawner>();
  addChild(std::move(asset_spawner));
  
  // Add script editor
  auto scriptEditor = std::make_unique<ScriptEditor>();
  auto scriptEditorPtr = scriptEditor.get();
  addChild(std::move(scriptEditor));
  
  // Link script editor to sidebar
  if (sidebarPtr) {
    sidebarPtr->scriptEditor = scriptEditorPtr;
  }
  
  name = "Gui";
  io = ImGui::GetIO();
  io.Fonts->Clear();
  auto font =
      io.Fonts->AddFontFromFileTTF("../assets/fonts/Orbit-Regular.ttf", 16.0f);
  io.Fonts->Build();
  ImGui::StyleColorsLight(); // Light theme
  ImGuiStyle &style = ImGui::GetStyle();
  style.FrameRounding = 8;
  style.ChildRounding = 8;
  style.GrabRounding = 8;
  style.FrameBorderSize = 1;
  style.TreeLinesSize = 1;
  style.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesFull;
  style.WindowRounding = 8;
};

void Gui::setModelManager(ModelManager* manager) {
  auto assetSpawner = getChildOfType<AssetSpawner>();
  if (assetSpawner) {
    assetSpawner->setModelManager(manager);
  }
}
} // namespace moiras
