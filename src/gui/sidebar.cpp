// filepicker.cpp
#include "sidebar.h"

namespace moiras {

Sidebar::Sidebar()
    : GameObject("Sidebar"), sidebarWidth(300.0f), currentTab(0) {}

void Sidebar::update() {
    GameObject::update();
    sidebarWidth = GetScreenWidth() * 0.2f;
    if (sidebarWidth < 200.0f) sidebarWidth = 200.0f;
    if (sidebarWidth > 400.0f) sidebarWidth = 400.0f;
}

void Sidebar::gui() {
    GameObject::gui();
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(sidebarWidth, GetScreenHeight()));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse;
    
    ImGui::Begin("Inspector", nullptr, flags);
    
    if (ImGui::BeginTabBar("SidebarTabs")) {
        if (ImGui::BeginTabItem("Scene")) {
            drawSceneTab(getRoot());
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Lighting")) {
            drawLightingTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Assets")) {
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Settings")) {
            drawSettingsTab();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
    
    // Show demo windows if enabled
    if (showImGuiDemo) {
        ImGui::ShowDemoWindow(&showImGuiDemo);
    }
    if (showStyleEditor) {
        ImGui::Begin("Style Editor", &showStyleEditor);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }
}

void Sidebar::drawSceneTab(GameObject *root) {
    if (!root) return;
    
    ImGui::Text("Scene Hierarchy");
    ImGui::Separator();
    drawGameObjectTree(root);
}

void Sidebar::drawLightingTab() {
    if (!lightManager) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "LightManager not set!");
        return;
    }
    
    lightManager->gui();
}

void Sidebar::drawSettingsTab() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    ImGui::Text("ImGui Settings");
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Style", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
        ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
        ImGui::SliderFloat("Child Rounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
        ImGui::SliderFloat("Grab Rounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
        ImGui::SliderFloat("Scrollbar Rounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
        
        ImGui::Spacing();
        ImGui::SliderFloat("Frame Border Size", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
        ImGui::SliderFloat("Window Border Size", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
        
        ImGui::Spacing();
        ImGui::SliderFloat2("Window Padding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat2("Frame Padding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat2("Item Spacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
    }
    
    if (ImGui::CollapsingHeader("Colors")) {
        ImGui::Text("Color Theme:");
        if (ImGui::Button("Dark")) ImGui::StyleColorsDark();
        ImGui::SameLine();
        if (ImGui::Button("Light")) ImGui::StyleColorsLight();
        ImGui::SameLine();
        if (ImGui::Button("Classic")) ImGui::StyleColorsClassic();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::ColorEdit3("Window Background", (float*)&style.Colors[ImGuiCol_WindowBg]);
        ImGui::ColorEdit3("Text", (float*)&style.Colors[ImGuiCol_Text]);
        ImGui::ColorEdit3("Border", (float*)&style.Colors[ImGuiCol_Border]);
        ImGui::ColorEdit3("Button", (float*)&style.Colors[ImGuiCol_Button]);
        ImGui::ColorEdit3("Button Hovered", (float*)&style.Colors[ImGuiCol_ButtonHovered]);
        ImGui::ColorEdit3("Button Active", (float*)&style.Colors[ImGuiCol_ButtonActive]);
        ImGui::ColorEdit3("Header", (float*)&style.Colors[ImGuiCol_Header]);
        ImGui::ColorEdit3("Header Hovered", (float*)&style.Colors[ImGuiCol_HeaderHovered]);
        ImGui::ColorEdit3("Header Active", (float*)&style.Colors[ImGuiCol_HeaderActive]);
    }
    
    if (ImGui::CollapsingHeader("Rendering")) {
        bool vsync = IsWindowState(FLAG_VSYNC_HINT);
        if (ImGui::Checkbox("VSync", &vsync)) {
            if (vsync) SetWindowState(FLAG_VSYNC_HINT);
            else ClearWindowState(FLAG_VSYNC_HINT);
        }
        
        ImGui::Text("Current FPS: %d", GetFPS());
        
        static int fpsTarget = 60;
        if (ImGui::SliderInt("Target FPS", &fpsTarget, 30, 144)) {
            SetTargetFPS(fpsTarget);
        }
    }
    
    if (ImGui::CollapsingHeader("Debug")) {
        if (ImGui::Button("Show ImGui Demo")) {
            showImGuiDemo = !showImGuiDemo;
        }
        if (ImGui::Button("Show Style Editor")) {
            showStyleEditor = !showStyleEditor;
        }
    }
}

void Sidebar::drawGameObjectTree(GameObject *obj) {
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
