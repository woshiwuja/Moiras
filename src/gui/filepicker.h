// filepicker.h
#pragma once
#include "../game/game_object.h"
#include "imgui.h"

namespace moiras {

class Sidebar : public GameObject {
public:
    Sidebar();
    ~Sidebar() = default;
    
    void update() override;
    void gui() override;
    
private:
    void drawSceneTab(GameObject* obj);
    void drawGameObjectTree(GameObject* obj);
    
    float sidebarWidth;
    int currentTab;
};

} // namespace moiras
