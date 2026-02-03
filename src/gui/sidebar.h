#pragma once

#include "../game/game_object.h"
#include "../lights/lightmanager.h"
#include "../resources/model_manager.h"
#include "imgui.h"
#include <raylib.h>

namespace moiras
{
    class StructureBuilder; // Forward declaration
    class ScriptEditor;     // Forward declaration

    class Sidebar : public GameObject
    {
    public:
        Sidebar();

        void update() override;
        void gui() override;

        // Puntatore al light manager (settato dal Game)
        LightManager *lightManager = nullptr;

        // Puntatore al model manager (settato dal Game)
        ModelManager *modelManager = nullptr;

        // Puntatore al structure builder (settato dal Game)
        StructureBuilder *structureBuilder = nullptr;

        // Puntatore allo script editor
        ScriptEditor *scriptEditor = nullptr;

        bool showImGuiDemo = false;
        bool showStyleEditor = false;

    private:
        float sidebarWidth;
        int currentTab;
        bool isOpen;
        void drawSceneTab(GameObject *root);
        void drawLightingTab();
        void drawBuildingTab();
        void drawScriptingTab();
        void drawSettingsTab();
        void drawGameObjectTree(GameObject *obj);
    };

} // namespace moiras
