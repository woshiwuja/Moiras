#include "gui.h"
#include "sidebar.h"
#include <imgui.h>
#include "asset_spawner.h"

namespace moiras {

Gui::~Gui() {}

Gui::Gui() {

  auto sidebar = std::make_unique<Sidebar>();
  addChild(std::move(sidebar));
  auto asset_spawner = std::make_unique<AssetSpawner>();
  addChild(std::move(asset_spawner));
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
} // namespace moiras
