#include "gui.h"
#include "../lights/lightmanager.h"
#include "asset_spawner.h"
#include "sidebar.h"
#include <imgui.h>
#include <raylib.h>

namespace moiras {

// Static member initialization
std::map<int, ImFont*> Gui::editorFonts;

Gui::~Gui() {}

Gui::Gui() {
  auto sidebar = std::make_unique<Sidebar>();
  addChild(std::move(sidebar));
  
  auto asset_spawner = std::make_unique<AssetSpawner>();
  addChild(std::move(asset_spawner));
  
  name = "Gui";
  io = ImGui::GetIO();
  io.Fonts->Clear();
  
  // Load base UI font at 16px (default)
  auto font = io.Fonts->AddFontFromFileTTF("../assets/fonts/Orbit-Regular.ttf", 16.0f);
  
  // Load editor fonts at various sizes for text editor
  const int fontSizes[] = {8, 10, 12, 14, 16, 18, 20, 22, 24, 28, 32};
  for (int size : fontSizes) {
    ImFont* editorFont = io.Fonts->AddFontFromFileTTF("../assets/fonts/Orbit-Regular.ttf", (float)size);
    if (editorFont) {
      editorFonts[size] = editorFont;
      TraceLog(LOG_INFO, "GUI: Loaded editor font size %dpx", size);
    }
  }
  
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

ImFont* Gui::getEditorFont(int size) {
  // Return exact match if available
  auto it = editorFonts.find(size);
  if (it != editorFonts.end()) {
    return it->second;
  }
  
  // Find closest font size
  int closestSize = 16; // Default
  int minDiff = 9999;
  for (const auto& pair : editorFonts) {
    int diff = std::abs(pair.first - size);
    if (diff < minDiff) {
      minDiff = diff;
      closestSize = pair.first;
    }
  }
  
  return editorFonts[closestSize];
}

void Gui::setModelManager(ModelManager* manager) {
  auto assetSpawner = getChildOfType<AssetSpawner>();
  if (assetSpawner) {
    assetSpawner->setModelManager(manager);
  }
}
} // namespace moiras
