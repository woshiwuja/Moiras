#pragma once

#include "../game/game_object.h"
#include "imgui.h"
#include <array>
#include <string>
#include <vector>

namespace moiras {

class Character; // Forward declaration

enum class EquipmentSlot : int {
    Head = 0,
    Chest,
    Legs,
    Feet,
    LeftHand,
    RightHand,
    COUNT
};

inline const char *getEquipmentSlotName(EquipmentSlot slot) {
    switch (slot) {
    case EquipmentSlot::Head:
        return "Head";
    case EquipmentSlot::Chest:
        return "Chest";
    case EquipmentSlot::Legs:
        return "Legs";
    case EquipmentSlot::Feet:
        return "Feet";
    case EquipmentSlot::LeftHand:
        return "L.Hand";
    case EquipmentSlot::RightHand:
        return "R.Hand";
    default:
        return "None";
    }
}

struct InventoryItem {
    std::string name;
    int gridX = -1, gridY = -1; // Position in grid (-1 = not placed)
    int width = 1, height = 1;  // Size in grid cells
    ImU32 color = IM_COL32(100, 140, 200, 255);
    EquipmentSlot compatibleSlot =
        EquipmentSlot::COUNT; // COUNT means not equippable
    int stackCount = 1;
    int maxStack = 1;
    bool equipped = false;
};

class Inventory : public GameObject {
  private:
    int m_gridCols;
    int m_gridRows;
    float m_slotSize = 48.0f;

    // Grid occupancy: -1 = empty, otherwise index into m_items
    std::vector<int> m_grid;
    std::vector<InventoryItem> m_items;

    // Equipment: -1 = no item equipped, otherwise index into m_items
    std::array<int, static_cast<int>(EquipmentSlot::COUNT)> m_equipped;

    // 3D character preview
    RenderTexture2D m_previewRT = {0};
    Camera3D m_previewCamera = {0};
    bool m_previewInitialized = false;
    float m_previewYaw = 0.0f;

    static constexpr int PREVIEW_W = 280;
    static constexpr int PREVIEW_H = 360;

    // Internal drawing helpers
    void initPreview();
    void renderCharacterPreview();
    void drawPreviewZone();
    void drawEquipmentZone();
    void drawGridZone();
    void drawEquipSlotButton(const char *label, EquipmentSlot slot, ImVec2 size);

    // Grid helpers
    void markGrid(int gx, int gy, int w, int h, int value);
    void clearItemFromGrid(int itemIdx);
    int findFirstFreePosition(int w, int h) const;

  public:
    bool isOpen = false;

    Inventory(int cols, int rows);
    ~Inventory();

    bool canPlace(int gx, int gy, int w, int h, int ignoreIdx = -1) const;
    int addItem(InventoryItem item);
    bool removeItem(int itemIdx);
    bool equipItem(int itemIdx);
    bool unequipItem(EquipmentSlot slot);

    int itemCount() const { return static_cast<int>(m_items.size()); }
    const InventoryItem *getItem(int idx) const;

    void update() override;
    void gui() override;
};

} // namespace moiras
