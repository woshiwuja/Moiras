// filepicker.cpp
#include "sidebar.h"
#include "../building/structure_builder.h"
#include "../map/environment.hpp"
#include "../time/time_manager.h"
#include "script_editor.h"
#include "../../rlImGui/rlImGui.h"

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

                if (BeginTabItem("Building"))
                {
                    drawBuildingTab();
                    EndTabItem();
                }

                if (BeginTabItem("Scripting"))
                {
                    drawScriptingTab();
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

    void Sidebar::drawBuildingTab()
    {
        if (!structureBuilder)
        {
            TextColored(ImVec4(1, 0, 0, 1), "StructureBuilder not set!");
            return;
        }

        bool buildingMode = structureBuilder->isBuildingMode();

        // Building mode status and controls (shown when active)
        if (buildingMode)
        {
            TextColored(ImVec4(0, 1, 0, 1), "BUILDING MODE ACTIVE");
            Separator();
            Text("Q/E: Rotate");
            Text("Shift+Scroll: Scale");
            Text("Left Click: Place");
            Text("ESC/Right Click: Exit");
            Separator();

            if (Button("Exit Building Mode", ImVec2(-1, 30)))
            {
                structureBuilder->exitBuildingMode();
            }

            Spacing();
        }
        else
        {
            Text("Select an asset to build:");
        }

        Separator();

        // Preview dell'asset selezionato (always visible)
        Texture2D previewTex = structureBuilder->getPreviewTexture();
        if (previewTex.id != 0)
        {
            float previewSize = sidebarWidth - 40;
            if (previewSize > 180) previewSize = 180;

            rlImGuiImageRect(&previewTex,
                           (int)previewSize, (int)(previewSize * 0.75f),
                           (Rectangle){0, 0, (float)previewTex.width, (float)previewTex.height});
        }
        else
        {
            // Placeholder grigio
            ImVec2 cursorPos = GetCursorScreenPos();
            float previewW = sidebarWidth - 40;
            if (previewW > 180) previewW = 180;
            float previewH = previewW * 0.75f;

            GetWindowDrawList()->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + previewW, cursorPos.y + previewH),
                IM_COL32(60, 60, 60, 255)
            );
            GetWindowDrawList()->AddRect(
                cursorPos,
                ImVec2(cursorPos.x + previewW, cursorPos.y + previewH),
                IM_COL32(100, 100, 100, 255)
            );

            ImVec2 textSize = CalcTextSize("No Selection");
            SetCursorScreenPos(ImVec2(
                cursorPos.x + (previewW - textSize.x) * 0.5f,
                cursorPos.y + (previewH - textSize.y) * 0.5f
            ));
            TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No Selection");
            SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + previewH + 5));
        }

        Spacing();

        // Asset list (always visible)
        Text("Available Assets:");

        const auto& assetList = structureBuilder->getAssetList();
        int selectedIndex = structureBuilder->getSelectedAssetIndex();

        // Lista scrollabile
        float listHeight = buildingMode ? 150.0f : 200.0f; // Smaller when in build mode
        BeginChild("AssetList", ImVec2(0, listHeight), true);

        for (size_t i = 0; i < assetList.size(); i++)
        {
            bool isSelected = (selectedIndex == (int)i);
            if (Selectable(assetList[i].c_str(), isSelected))
            {
                structureBuilder->selectAsset((int)i);
                // If in building mode, switch to the new asset
                if (buildingMode)
                {
                    structureBuilder->exitBuildingMode();
                    structureBuilder->enterBuildingMode();
                }
            }
        }

        EndChild();

        Spacing();

        // Pulsante refresh
        if (Button("Refresh List", ImVec2(-1, 0)))
        {
            structureBuilder->refreshAssetList();
        }

        // Show Start Building button only when not already in building mode
        if (!buildingMode)
        {
            Spacing();

            bool canBuild = (selectedIndex >= 0);
            if (!canBuild)
            {
                PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
            }

            if (Button("Start Building", ImVec2(-1, 35)) && canBuild)
            {
                structureBuilder->enterBuildingMode();
            }

            if (!canBuild)
            {
                PopStyleVar();
                TextColored(ImVec4(1, 0.5f, 0, 1), "Select an asset first");
            }
        }
    }

    void Sidebar::drawSettingsTab()
    {
        ImGuiStyle &style = GetStyle();

        // Game Speed Controls
        if (CollapsingHeader("Game Speed", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& timeMgr = TimeManager::getInstance();
            
            // Status display
            if (timeMgr.isPaused()) {
                PushStyleColor(ImGuiCol_Text, IM_COL32(255, 80, 80, 255));
                Text("Status: PAUSED");
                PopStyleColor();
            } else {
                float speed = timeMgr.getTimeScale();
                Text("Status: Running at %.1fx", speed);
            }
            
            Separator();
            
            // Pause button (full width)
            bool isPaused = timeMgr.isPaused();
            if (Button(isPaused ? "Resume (Space)" : "Pause (Space)", 
                      ImVec2(-1, 40))) {
                timeMgr.togglePause();
            }
            
            Spacing();
            
            // Speed buttons (3 in a row)
            BeginDisabled(isPaused);
            
            float currentSpeed = timeMgr.getTimeScale();
            ImVec4 activeColor = ImVec4(0.3f, 0.7f, 0.3f, 1.0f);
            float btnWidth = (GetContentRegionAvail().x - 8) / 3.0f;
            
            if (currentSpeed == 1.0f && !isPaused) {
                PushStyleColor(ImGuiCol_Button, activeColor);
            }
            if (Button("1x (1)", ImVec2(btnWidth, 30))) {
                timeMgr.setTimeScale(1.0f);
            }
            if (currentSpeed == 1.0f && !isPaused) {
                PopStyleColor();
            }
            
            SameLine();
            
            if (currentSpeed == 2.5f && !isPaused) {
                PushStyleColor(ImGuiCol_Button, activeColor);
            }
            if (Button("2.5x (2)", ImVec2(btnWidth, 30))) {
                timeMgr.setTimeScale(2.5f);
            }
            if (currentSpeed == 2.5f && !isPaused) {
                PopStyleColor();
            }
            
            SameLine();
            
            if (currentSpeed == 5.0f && !isPaused) {
                PushStyleColor(ImGuiCol_Button, activeColor);
            }
            if (Button("5x (3)", ImVec2(btnWidth, 30))) {
                timeMgr.setTimeScale(5.0f);
            }
            if (currentSpeed == 5.0f && !isPaused) {
                PopStyleColor();
            }
            
            EndDisabled();
            
            Spacing();
            TextWrapped("Hotkeys work in-game. Camera and UI are not affected by speed.");
        }

        Spacing();
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

            Spacing();
            Separator();
            Spacing();

            // Outline shader toggle
            if (this->outlineEnabled != nullptr)
            {
                Checkbox("Outline Shader", this->outlineEnabled);
            }
            else
            {
                TextColored(ImVec4(1, 0.5f, 0, 1), "Outline toggle not set");
            }

            // Shadow toggle
            if (this->lightManager != nullptr)
            {
                Checkbox("Shadows", &this->lightManager->shadowsEnabled);
            }
        }

        if (CollapsingHeader("Environment"))
        {
            if (environmentObject)
            {
                Text("Instanced Rocks");
                Separator();

                Text("Total instances: %d", environmentObject->getTotalInstanceCount());
                Text("Patches: %d", environmentObject->getPatchCount());
                Checkbox("Visible##rocks", &environmentObject->isVisible);

                // Cull distance
                float cullDist = environmentObject->getCullDistance();
                if (SliderFloat("Cull Distance", &cullDist, 20.0f, 500.0f))
                {
                    environmentObject->setCullDistance(cullDist);
                }

                // Patch info
                for (int i = 0; i < environmentObject->getPatchCount(); i++)
                {
                    auto &p = environmentObject->getPatch(i);
                    bool isActive = (i == environmentObject->getActivePatch());
                    if (isActive) PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                    Text("  [%d] %s: %d", i, rockMeshTypeName(p.meshType), (int)p.transforms.size());
                    if (isActive) PopStyleColor();
                }

                Spacing();
                Separator();
                Text("Active Brush Mesh:");
                int currentType = (int)environmentObject->getActiveMeshType();
                for (int i = 0; i < (int)RockMeshType::COUNT; i++)
                {
                    if (RadioButton(rockMeshTypeName((RockMeshType)i), &currentType, i))
                    {
                        environmentObject->setActiveMeshType((RockMeshType)i);
                    }
                }

                Spacing();
                Separator();
                Text("Brush Tool");
                Spacing();

                bool brushActive = environmentObject->isBrushMode();
                if (brushActive) {
                    PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                }
                if (Button(brushActive ? "Brush: ON" : "Brush: OFF", ImVec2(-1, 30)))
                {
                    environmentObject->setBrushMode(!brushActive);
                }
                if (brushActive) {
                    PopStyleColor();
                }

                if (brushActive) {
                    TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Left Click: Paint");
                    TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Right Click: Erase");
                }

                float brushRadius = environmentObject->getBrushRadius();
                if (SliderFloat("Brush Radius", &brushRadius, 2.0f, 50.0f))
                {
                    environmentObject->setBrushRadius(brushRadius);
                }

                int brushDensity = environmentObject->getBrushDensity();
                if (SliderInt("Brush Density", &brushDensity, 1, 30))
                {
                    environmentObject->setBrushDensity(brushDensity);
                }

                Spacing();
                if (Button("Clear All Rocks", ImVec2(-1, 0)))
                {
                    environmentObject->clearAll();
                }
            }
            else
            {
                TextColored(ImVec4(1, 0.5f, 0, 1), "EnvironmentalObject not set");
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

        if (CollapsingHeader("Model Cache"))
        {
            if (modelManager)
            {
                modelManager->gui();
            }
            else
            {
                TextColored(ImVec4(1, 0.5f, 0, 1), "ModelManager not set");
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

    void Sidebar::drawScriptingTab()
    {
        Text("Lua Script Editor");
        Separator();
        Spacing();

        Text("Open the integrated Ned editor to edit");
        Text("Lua scripts for your game objects.");
        Spacing();

        if (!scriptEditor)
        {
            TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Script editor not available");
            Text("The editor will be initialized on first use.");
            return;
        }

        bool isEditorOpen = scriptEditor->isOpen();
        
        if (Button(isEditorOpen ? "Hide Script Editor" : "Open Script Editor", ImVec2(-1, 40)))
        {
            scriptEditor->setOpen(!isEditorOpen);
        }

        Spacing();
        Separator();
        Spacing();

        if (CollapsingHeader("Scripts Directory", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Text("Scripts Path:");
            TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "../assets/scripts");
            
            Spacing();
            
            if (Button("Refresh Scripts", ImVec2(-1, 0)))
            {
                TraceLog(LOG_INFO, "SCRIPTING: Refreshing scripts directory");
            }
        }

        if (CollapsingHeader("Hot Reload"))
        {
            Text("Scripts are automatically reloaded");
            Text("when changes are detected.");
            
            Spacing();
            
            Text("Hot reload check interval:");
            TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Every 60 frames (~1 sec)");
        }

        if (CollapsingHeader("Help"))
        {
            TextWrapped("The Ned editor provides syntax highlighting, "
                       "LSP support, multi-cursor editing, and more.");
            
            Spacing();
            
            TextWrapped("Use Ctrl+S to save files, Ctrl+F to search, "
                       "and Ctrl+P for the command palette.");
        }
    }
} // namespace moiras