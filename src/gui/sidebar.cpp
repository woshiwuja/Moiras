// filepicker.cpp
#include "sidebar.h"

using namespace ImGui;
namespace moiras
{

    Sidebar::Sidebar()
        : GameObject("Sidebar"), sidebarWidth(300.0f), currentTab(0), isOpen(true) {}

    void Sidebar::update()
    {
        GameObject::update();
        sidebarWidth = GetScreenWidth() * 0.2f;
        if (sidebarWidth < 200.0f)
            sidebarWidth = 200.0f;
        if (sidebarWidth > 400.0f)
            sidebarWidth = 400.0f;
    }

    void Sidebar::gui()
    {
        GameObject::gui();

        const float tabWidth = 30.0f;
        const float tabHeight = 80.0f;

        // Animazione smooth
        static float currentX = 0.0f;
        float targetX = isOpen ? 0.0f : -sidebarWidth;
        float animSpeed = 10.0f * GetFrameTime();
        currentX += (targetX - currentX) * animSpeed;

        // Tab button (sempre visibile)
        float tabX = currentX + sidebarWidth;
        float tabY = GetScreenHeight() / 2.0f - tabHeight / 2.0f;

        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
        SetNextWindowPos(ImVec2(tabX, tabY));
        SetNextWindowSize(ImVec2(tabWidth, tabHeight));
        Begin("##SidebarTab", nullptr,
              ImGuiWindowFlags_NoTitleBar |
                  ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoScrollbar |
                  ImGuiWindowFlags_NoBackground);

        if (Button(isOpen ? "<##toggle" : ">##toggle", ImVec2(tabWidth - 4, tabHeight - 4)))
        {
            isOpen = !isOpen;
        }

        End();
        PopStyleVar();

        // Sidebar principale (solo se visibile)
        if (currentX > -sidebarWidth + 5)
        {
            SetNextWindowPos(ImVec2(currentX, 0));
            SetNextWindowSize(ImVec2(sidebarWidth, GetScreenHeight()));

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoCollapse;

            Begin("Inspector", nullptr, flags);

            if (BeginTabBar("SidebarTabs"))
            {
                if (BeginTabItem("Scene"))
                {
                    drawSceneTab(getRoot());
                    EndTabItem();
                }

                if (BeginTabItem("Lighting"))
                {
                    drawLightingTab();
                    EndTabItem();
                }

                if (BeginTabItem("Assets"))
                {
                    EndTabItem();
                }

                if (BeginTabItem("Settings"))
                {
                    drawSettingsTab();
                    EndTabItem();
                }

                EndTabBar();
            }

            End();
        }

        // Show demo windows if enabled
        if (showImGuiDemo)
        {
            ShowDemoWindow(&showImGuiDemo);
        }
        if (showStyleEditor)
        {
            Begin("Style Editor", &showStyleEditor);
            ShowStyleEditor();
            End();
        }
    }

    void Sidebar::drawSceneTab(GameObject *root)
    {
        if (!root)
            return;

        Text("Scene Hierarchy");
        Separator();
        drawGameObjectTree(root);
    }

    void Sidebar::drawLightingTab()
    {
        if (!lightManager)
        {
            TextColored(ImVec4(1, 0, 0, 1), "LightManager not set!");
            return;
        }

        lightManager->gui();
    }

    void Sidebar::drawSettingsTab()
    {
        ImGuiStyle &style = GetStyle();

        Text("ImGui Settings");
        Separator();

        if (CollapsingHeader("Style", ImGuiTreeNodeFlags_DefaultOpen))
        {
            SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("Child Rounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("Grab Rounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("Scrollbar Rounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");

            Spacing();
            SliderFloat("Frame Border Size", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
            SliderFloat("Window Border Size", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");

            Spacing();
            SliderFloat2("Window Padding", (float *)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
            SliderFloat2("Frame Padding", (float *)&style.FramePadding, 0.0f, 20.0f, "%.0f");
            SliderFloat2("Item Spacing", (float *)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
        }

        if (CollapsingHeader("Colors"))
        {
            Text("Color Theme:");
            if (Button("Dark"))
                StyleColorsDark();
            SameLine();
            if (Button("Light"))
                StyleColorsLight();
            SameLine();
            if (Button("Classic"))
                StyleColorsClassic();

            Spacing();
            Separator();
            Spacing();

            ColorEdit3("Window Background", (float *)&style.Colors[ImGuiCol_WindowBg]);
            ColorEdit3("Text", (float *)&style.Colors[ImGuiCol_Text]);
            ColorEdit3("Border", (float *)&style.Colors[ImGuiCol_Border]);
            ColorEdit3("Button", (float *)&style.Colors[ImGuiCol_Button]);
            ColorEdit3("Button Hovered", (float *)&style.Colors[ImGuiCol_ButtonHovered]);
            ColorEdit3("Button Active", (float *)&style.Colors[ImGuiCol_ButtonActive]);
            ColorEdit3("Header", (float *)&style.Colors[ImGuiCol_Header]);
            ColorEdit3("Header Hovered", (float *)&style.Colors[ImGuiCol_HeaderHovered]);
            ColorEdit3("Header Active", (float *)&style.Colors[ImGuiCol_HeaderActive]);
        }

        if (CollapsingHeader("Rendering"))
        {
            bool vsync = IsWindowState(FLAG_VSYNC_HINT);
            if (Checkbox("VSync", &vsync))
            {
                if (vsync)
                    SetWindowState(FLAG_VSYNC_HINT);
                else
                    ClearWindowState(FLAG_VSYNC_HINT);
            }

            Text("Current FPS: %d", GetFPS());

            static int fpsTarget = 60;
            if (SliderInt("Target FPS", &fpsTarget, 30, 144))
            {
                SetTargetFPS(fpsTarget);
            }
        }

        if (CollapsingHeader("Debug"))
        {
            if (Button("Show ImGui Demo"))
            {
                showImGuiDemo = !showImGuiDemo;
            }
            if (Button("Show Style Editor"))
            {
                showStyleEditor = !showStyleEditor;
            }
        }
    }

    void Sidebar::drawGameObjectTree(GameObject *obj)
    {
        if (!obj)
            return;

        bool hasChildren = obj->getChildCount() > 0;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;

        if (!hasChildren)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        bool nodeOpen = TreeNodeEx(obj->getNameC(), flags);

        if (nodeOpen && hasChildren)
        {
            for (size_t i = 0; i < obj->getChildCount(); i++)
            {
                drawGameObjectTree(obj->getChildAt(i));
            }
            TreePop();
        }
    }
} // namespace moiras