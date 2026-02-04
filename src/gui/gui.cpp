#include "gui.h"
#include "../lights/lightmanager.h"
#include "../time/time_manager.h"
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

void Gui::gui() {
  // Call parent gui (draws children: Sidebar, ScriptEditor, etc.)
  GameObject::gui();
  
  // Draw BIG speed control buttons
  auto& timeMgr = TimeManager::getInstance();
  bool isPaused = timeMgr.isPaused();
  float speed = timeMgr.getTimeScale();
  
  ImGuiIO& io = ImGui::GetIO();
  
  // Position in top-right corner
  const float buttonWidth = 120.0f;
  const float buttonHeight = 60.0f;
  const float spacing = 10.0f;
  const float totalWidth = buttonWidth * 4 + spacing * 3 + 20; // 4 buttons + spacing + padding
  
  ImVec2 windowPos = ImVec2(io.DisplaySize.x - totalWidth - 20, 20);
  
  ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(totalWidth, buttonHeight + 40), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.85f);
  
  ImGui::Begin("##SpeedControl", nullptr, 
      ImGuiWindowFlags_NoDecoration | 
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoResize);
  
  // Style: larger font and colors
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 15));
  
  // Pause button
  if (isPaused) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
  }
  
  if (ImGui::Button("PAUSE\n(Space)", ImVec2(buttonWidth, buttonHeight))) {
    timeMgr.setPaused(!isPaused);
  }
  ImGui::PopStyleColor(3);
  
  ImGui::SameLine(0, spacing);
  
  // 1x button
  bool isNormal = !isPaused && speed == 1.0f;
  if (isNormal) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
  }
  
  if (ImGui::Button("1x\n(1)", ImVec2(buttonWidth, buttonHeight))) {
    timeMgr.setTimeScale(1.0f);
  }
  ImGui::PopStyleColor(3);
  
  ImGui::SameLine(0, spacing);
  
  // 2.5x button
  bool isMedium = !isPaused && speed == 2.5f;
  if (isMedium) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.6f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.4f, 0.1f, 1.0f));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
  }
  
  if (ImGui::Button("2.5x\n(2)", ImVec2(buttonWidth, buttonHeight))) {
    timeMgr.setTimeScale(2.5f);
  }
  ImGui::PopStyleColor(3);
  
  ImGui::SameLine(0, spacing);
  
  // 5x button
  bool isFast = !isPaused && speed == 5.0f;
  if (isFast) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.3f, 0.1f, 1.0f));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
  }
  
  if (ImGui::Button("5x\n(3)", ImVec2(buttonWidth, buttonHeight))) {
    timeMgr.setTimeScale(5.0f);
  }
  ImGui::PopStyleColor(3);
  
  ImGui::PopStyleVar(2);
  
  ImGui::End();
}

void Gui::setModelManager(ModelManager* manager) {
  auto assetSpawner = getChildOfType<AssetSpawner>();
  if (assetSpawner) {
    assetSpawner->setModelManager(manager);
  }
}
} // namespace moiras
