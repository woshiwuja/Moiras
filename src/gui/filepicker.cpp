// filepicker.cpp
#include "filepicker.h"

namespace moiras {

Sidebar::Sidebar()
    : GameObject("Sidebar"), sidebarWidth(300.0f), currentTab(0) {}

void Sidebar::update() {
  GameObject::update();

  // Update sidebar width based on window size
  sidebarWidth = GetScreenWidth() * 0.2f; // 20% of screen width
  if (sidebarWidth < 200.0f)
    sidebarWidth = 200.0f;
  if (sidebarWidth > 400.0f)
    sidebarWidth = 400.0f;
}

void Sidebar::gui() {
  GameObject::gui();

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(sidebarWidth, GetScreenHeight()));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse;

  ImGui::Begin("Inspector", nullptr, flags);

  // Tabs
  if (ImGui::BeginTabBar("SidebarTabs")) {
    if (ImGui::BeginTabItem("Scene")) {
      drawSceneTab(getParent());
      ImGui::EndTabItem();
    }

    // Add more tabs here later
    // if (ImGui::BeginTabItem("Assets")) { ... }
    // if (ImGui::BeginTabItem("Settings")) { ... }

    ImGui::EndTabBar();
  }

  ImGui::End();
}

void Sidebar::drawSceneTab(GameObject *root) {
  if (!root)
    return;

  ImGui::Text("Scene Hierarchy");
  ImGui::Separator();

  drawGameObjectTree(root);
}

void Sidebar::drawGameObjectTree(GameObject* obj) {
    if (!obj) return;
    
    bool hasChildren = obj->getChildCount() > 0;
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_SpanAvailWidth;
    
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    
    bool nodeOpen = ImGui::TreeNodeEx(obj->getNameC(), flags);
    
    if (nodeOpen && hasChildren) {
        for (size_t i = 0; i < obj->getChildCount(); i++) {
            drawGameObjectTree(obj->getChildAt(i));
        }
        ImGui::TreePop();
    }
}
} // namespace moiras
