#pragma once

#include "../game/game_object.h"
#include "../lights/lightmanager.h"
#include "imgui.h"
#include <raylib.h>

namespace moiras {

class Sidebar : public GameObject {
public:
    Sidebar();
    
    void update() override;
    void gui() override;
    
    // Puntatore al light manager (settato dal Game)
    LightManager* lightManager = nullptr;
    
    bool showImGuiDemo = false;
    bool showStyleEditor = false;

private:
    float sidebarWidth;
    int currentTab;
    
    void drawSceneTab(GameObject* root);
    void drawLightingTab();
    void drawSettingsTab();
    void drawGameObjectTree(GameObject* obj);
};

} // namespace moiras
