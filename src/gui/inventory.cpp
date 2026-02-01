#include "inventory.hpp"
#include "../character/character.h"
#include "../../rlImGui/rlImGui.h"
#include <algorithm>
#include <cmath>
#include <raylib.h>
#include <raymath.h>

namespace moiras {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

Inventory::Inventory(int cols, int rows)
    : m_gridCols(cols), m_gridRows(rows) {
    m_grid.resize(m_gridCols * m_gridRows, -1);
    m_equipped.fill(-1);

    // Seed a few test items so the inventory isn't empty on first open
    InventoryItem sword;
    sword.name = "Iron Sword";
    sword.width = 1;
    sword.height = 3;
    sword.color = IM_COL32(180, 180, 180, 255);
    sword.compatibleSlot = EquipmentSlot::RightHand;
    addItem(std::move(sword));

    InventoryItem helmet;
    helmet.name = "Leather Helmet";
    helmet.width = 2;
    helmet.height = 2;
    helmet.color = IM_COL32(139, 90, 43, 255);
    helmet.compatibleSlot = EquipmentSlot::Head;
    addItem(std::move(helmet));

    InventoryItem bread;
    bread.name = "Bread";
    bread.width = 1;
    bread.height = 1;
    bread.color = IM_COL32(210, 180, 120, 255);
    bread.stackCount = 3;
    bread.maxStack = 10;
    addItem(std::move(bread));

    InventoryItem chainmail;
    chainmail.name = "Chainmail";
    chainmail.width = 2;
    chainmail.height = 3;
    chainmail.color = IM_COL32(160, 160, 170, 255);
    chainmail.compatibleSlot = EquipmentSlot::Chest;
    addItem(std::move(chainmail));

    InventoryItem boots;
    boots.name = "Leather Boots";
    boots.width = 2;
    boots.height = 2;
    boots.color = IM_COL32(110, 70, 40, 255);
    boots.compatibleSlot = EquipmentSlot::Feet;
    addItem(std::move(boots));
}

Inventory::~Inventory() {
    if (m_previewRT.id > 0) {
        UnloadRenderTexture(m_previewRT);
    }
}

// ---------------------------------------------------------------------------
// 3D preview
// ---------------------------------------------------------------------------

void Inventory::initPreview() {
    if (m_previewInitialized)
        return;

    m_previewRT = LoadRenderTexture(PREVIEW_W, PREVIEW_H);

    m_previewCamera.position = {0.0f, 1.0f, 3.0f};
    m_previewCamera.target = {0.0f, 0.8f, 0.0f};
    m_previewCamera.up = {0.0f, 1.0f, 0.0f};
    m_previewCamera.fovy = 30.0f;
    m_previewCamera.projection = CAMERA_PERSPECTIVE;

    m_previewInitialized = true;
}

void Inventory::renderCharacterPreview() {
    auto *parent = dynamic_cast<Character *>(getParent());
    if (!parent || !parent->hasModel())
        return;

    if (!m_previewInitialized)
        initPreview();

    auto &mi = parent->modelInstance;

    // Auto-frame the model based on its bounding box
    BoundingBox bbox = mi.getBoundingBox();
    float modelHeight = (bbox.max.y - bbox.min.y) * parent->scale;
    float modelCenterY = ((bbox.max.y + bbox.min.y) / 2.0f) * parent->scale;

    float dist = 3.0f;
    if (modelHeight > 0.01f) {
        dist = (modelHeight * 1.5f) /
               tanf(m_previewCamera.fovy * DEG2RAD * 0.5f);
        dist = std::clamp(dist, 0.5f, 100.0f);
    }

    float camX = sinf(m_previewYaw) * dist;
    float camZ = cosf(m_previewYaw) * dist;
    m_previewCamera.position = {camX, modelCenterY + modelHeight * 0.15f, camZ};
    m_previewCamera.target = {0.0f, modelCenterY, 0.0f};

    // Render to texture
    BeginTextureMode(m_previewRT);
    ClearBackground(BLACK);
    BeginMode3D(m_previewCamera);

    // Bind per-instance animation data so preview matches the in-world pose
    if (mi.hasAnimationData()) {
        mi.bindAnimationData();
    }

    Matrix transform = MatrixScale(parent->scale, parent->scale, parent->scale);

    for (int i = 0; i < mi.meshCount(); i++) {
        int matIdx = mi.meshMaterial()[i];
        DrawMesh(mi.meshes()[i], mi.materials()[matIdx], transform);
    }

    if (mi.hasAnimationData()) {
        mi.unbindAnimationData();
    }

    EndMode3D();
    EndTextureMode();
}

// ---------------------------------------------------------------------------
// Grid helpers
// ---------------------------------------------------------------------------

bool Inventory::canPlace(int gx, int gy, int w, int h, int ignoreIdx) const {
    if (gx < 0 || gy < 0 || gx + w > m_gridCols || gy + h > m_gridRows)
        return false;

    for (int y = gy; y < gy + h; y++) {
        for (int x = gx; x < gx + w; x++) {
            int cellVal = m_grid[y * m_gridCols + x];
            if (cellVal != -1 && cellVal != ignoreIdx)
                return false;
        }
    }
    return true;
}

void Inventory::markGrid(int gx, int gy, int w, int h, int value) {
    for (int y = gy; y < gy + h; y++) {
        for (int x = gx; x < gx + w; x++) {
            m_grid[y * m_gridCols + x] = value;
        }
    }
}

void Inventory::clearItemFromGrid(int itemIdx) {
    auto &item = m_items[itemIdx];
    if (item.gridX >= 0 && item.gridY >= 0) {
        markGrid(item.gridX, item.gridY, item.width, item.height, -1);
        item.gridX = -1;
        item.gridY = -1;
    }
}

int Inventory::findFirstFreePosition(int w, int h) const {
    for (int y = 0; y <= m_gridRows - h; y++) {
        for (int x = 0; x <= m_gridCols - w; x++) {
            if (canPlace(x, y, w, h))
                return y * m_gridCols + x;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Item management
// ---------------------------------------------------------------------------

int Inventory::addItem(InventoryItem item) {
    int idx = static_cast<int>(m_items.size());

    int pos = findFirstFreePosition(item.width, item.height);
    if (pos < 0)
        return -1;

    item.gridX = pos % m_gridCols;
    item.gridY = pos / m_gridCols;

    m_items.push_back(std::move(item));
    markGrid(m_items[idx].gridX, m_items[idx].gridY, m_items[idx].width,
             m_items[idx].height, idx);
    return idx;
}

bool Inventory::removeItem(int itemIdx) {
    if (itemIdx < 0 || itemIdx >= static_cast<int>(m_items.size()))
        return false;

    auto &item = m_items[itemIdx];
    clearItemFromGrid(itemIdx);

    if (item.equipped) {
        for (int i = 0; i < static_cast<int>(EquipmentSlot::COUNT); i++) {
            if (m_equipped[i] == itemIdx) {
                m_equipped[i] = -1;
                break;
            }
        }
    }

    item.name.clear();
    item.gridX = -1;
    item.gridY = -1;
    item.equipped = false;
    return true;
}

bool Inventory::equipItem(int itemIdx) {
    if (itemIdx < 0 || itemIdx >= static_cast<int>(m_items.size()))
        return false;

    auto &item = m_items[itemIdx];
    if (item.compatibleSlot == EquipmentSlot::COUNT)
        return false;
    if (item.equipped)
        return false;

    int slotIdx = static_cast<int>(item.compatibleSlot);

    // Unequip existing item in that slot first
    if (m_equipped[slotIdx] != -1) {
        unequipItem(item.compatibleSlot);
    }

    clearItemFromGrid(itemIdx);
    m_equipped[slotIdx] = itemIdx;
    item.equipped = true;
    return true;
}

bool Inventory::unequipItem(EquipmentSlot slot) {
    int slotIdx = static_cast<int>(slot);
    if (m_equipped[slotIdx] == -1)
        return false;

    int itemIdx = m_equipped[slotIdx];
    auto &item = m_items[itemIdx];

    int pos = findFirstFreePosition(item.width, item.height);
    if (pos < 0)
        return false; // No grid space

    item.gridX = pos % m_gridCols;
    item.gridY = pos / m_gridCols;
    item.equipped = false;
    markGrid(item.gridX, item.gridY, item.width, item.height, itemIdx);

    m_equipped[slotIdx] = -1;
    return true;
}

const InventoryItem *Inventory::getItem(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(m_items.size()))
        return nullptr;
    return &m_items[idx];
}

// ---------------------------------------------------------------------------
// Update & GUI entry points
// ---------------------------------------------------------------------------

void Inventory::update() {
    if (!isOpen)
        return;
    renderCharacterPreview();
    GameObject::update();
}

void Inventory::gui() {
    if (IsKeyPressed(KEY_I))
        isOpen = !isOpen;
    if (!isOpen)
        return;

    ImGui::SetNextWindowSize(ImVec2(600, 720), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inventory", &isOpen, ImGuiWindowFlags_NoScrollbar);

    // ---- Top section: preview + equipment side by side ----
    float topHeight = static_cast<float>(PREVIEW_H) + 20.0f;
    ImGui::BeginChild("TopSection", ImVec2(0, topHeight), ImGuiChildFlags_None);

    drawPreviewZone();
    ImGui::SameLine();
    drawEquipmentZone();

    ImGui::EndChild();

    ImGui::Separator();

    // ---- Bottom section: grid inventory ----
    drawGridZone();

    ImGui::End();
}

// ---------------------------------------------------------------------------
// Preview zone
// ---------------------------------------------------------------------------

void Inventory::drawPreviewZone() {
    ImGui::BeginChild("PreviewZone",
                      ImVec2(static_cast<float>(PREVIEW_W) + 16.0f,
                             static_cast<float>(PREVIEW_H) + 16.0f),
                      ImGuiChildFlags_Borders);

    if (m_previewInitialized && m_previewRT.texture.id > 0) {
        rlImGuiImageRenderTextureFit(&m_previewRT, true);

        // Drag mouse over the preview to rotate the character model
        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            m_previewYaw += ImGui::GetIO().MouseDelta.x * 0.01f;
        }
    } else {
        ImGui::TextDisabled("No character model");
    }

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------
// Equipment zone
// ---------------------------------------------------------------------------

void Inventory::drawEquipmentZone() {
    ImGui::BeginChild("EquipmentZone", ImVec2(0, static_cast<float>(PREVIEW_H) + 16.0f),
                      ImGuiChildFlags_Borders);

    ImGui::TextUnformatted("Equipment");
    ImGui::Separator();
    ImGui::Spacing();

    ImVec2 slotSize(60, 60);

    // Vertical slots
    drawEquipSlotButton("Head", EquipmentSlot::Head, slotSize);
    drawEquipSlotButton("Chest", EquipmentSlot::Chest, slotSize);
    drawEquipSlotButton("Legs", EquipmentSlot::Legs, slotSize);
    drawEquipSlotButton("Feet", EquipmentSlot::Feet, slotSize);

    ImGui::Spacing();
    ImGui::TextUnformatted("Hands");
    drawEquipSlotButton("L.Hand", EquipmentSlot::LeftHand, slotSize);
    ImGui::SameLine();
    drawEquipSlotButton("R.Hand", EquipmentSlot::RightHand, slotSize);

    ImGui::EndChild();
}

void Inventory::drawEquipSlotButton(const char *label, EquipmentSlot slot,
                                    ImVec2 size) {
    int slotIdx = static_cast<int>(slot);
    int itemIdx = m_equipped[slotIdx];

    ImGui::PushID(static_cast<int>(slot));

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImDrawList *dl = ImGui::GetWindowDrawList();

    ImU32 bgColor = IM_COL32(40, 40, 50, 255);
    ImU32 borderColor = IM_COL32(100, 100, 120, 255);

    if (itemIdx != -1) {
        bgColor = m_items[itemIdx].color;
        borderColor = IM_COL32(220, 200, 100, 255);
    }

    dl->AddRectFilled(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y),
                      bgColor, 4.0f);
    dl->AddRect(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y),
                borderColor, 4.0f, 0, 1.5f);

    // Draw text
    if (itemIdx == -1) {
        ImVec2 textSize = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(cursor.x + (size.x - textSize.x) * 0.5f,
                           cursor.y + (size.y - textSize.y) * 0.5f),
                    IM_COL32(120, 120, 140, 255), label);
    } else {
        const char *itemName = m_items[itemIdx].name.c_str();
        dl->AddText(ImVec2(cursor.x + 3, cursor.y + 3),
                    IM_COL32(255, 255, 255, 255), itemName);
    }

    ImGui::InvisibleButton("##equipslot", size);

    // Drag source
    if (itemIdx != -1 &&
        ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("INV_ITEM", &itemIdx, sizeof(int));
        ImGui::Text("%s", m_items[itemIdx].name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload("INV_ITEM")) {
            int droppedIdx = *(const int *)payload->Data;
            auto &droppedItem = m_items[droppedIdx];

            if (droppedItem.compatibleSlot == slot) {
                // Unequip existing item in this slot
                if (m_equipped[slotIdx] != -1) {
                    unequipItem(slot);
                }
                // Remove dropped item from previous location
                if (droppedItem.equipped) {
                    for (int i = 0; i < static_cast<int>(EquipmentSlot::COUNT);
                         i++) {
                        if (m_equipped[i] == droppedIdx) {
                            m_equipped[i] = -1;
                            break;
                        }
                    }
                } else {
                    clearItemFromGrid(droppedIdx);
                }
                m_equipped[slotIdx] = droppedIdx;
                droppedItem.equipped = true;
                droppedItem.gridX = -1;
                droppedItem.gridY = -1;
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Right-click to unequip
    if (itemIdx != -1 && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        unequipItem(slot);
    }

    // Tooltip
    if (itemIdx != -1 && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", m_items[itemIdx].name.c_str());
        ImGui::TextDisabled("Right-click to unequip");
        ImGui::EndTooltip();
    }

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// Grid zone
// ---------------------------------------------------------------------------

void Inventory::drawGridZone() {
    ImGui::BeginChild("GridZone", ImVec2(0, 0), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::TextUnformatted("Backpack");
    ImGui::Separator();

    ImVec2 gridOrigin = ImGui::GetCursorScreenPos();
    ImDrawList *dl = ImGui::GetWindowDrawList();

    float gridW = m_gridCols * m_slotSize;
    float gridH = m_gridRows * m_slotSize;

    // Background
    dl->AddRectFilled(gridOrigin,
                      ImVec2(gridOrigin.x + gridW, gridOrigin.y + gridH),
                      IM_COL32(30, 30, 35, 255));

    // Grid lines
    for (int x = 0; x <= m_gridCols; x++) {
        float px = gridOrigin.x + x * m_slotSize;
        dl->AddLine(ImVec2(px, gridOrigin.y), ImVec2(px, gridOrigin.y + gridH),
                    IM_COL32(60, 60, 70, 255));
    }
    for (int y = 0; y <= m_gridRows; y++) {
        float py = gridOrigin.y + y * m_slotSize;
        dl->AddLine(ImVec2(gridOrigin.x, py), ImVec2(gridOrigin.x + gridW, py),
                    IM_COL32(60, 60, 70, 255));
    }

    // Drag highlight: show green/red overlay where the dragged item would land
    const ImGuiPayload *dragPayload = ImGui::GetDragDropPayload();
    bool isDragging =
        dragPayload && dragPayload->IsDataType("INV_ITEM") && dragPayload->Data;
    int dragIdx = -1;
    if (isDragging) {
        dragIdx = *(const int *)dragPayload->Data;
    }

    if (isDragging && dragIdx >= 0 &&
        dragIdx < static_cast<int>(m_items.size())) {
        ImVec2 mousePos = ImGui::GetMousePos();
        int hoverX =
            static_cast<int>((mousePos.x - gridOrigin.x) / m_slotSize);
        int hoverY =
            static_cast<int>((mousePos.y - gridOrigin.y) / m_slotSize);

        if (hoverX >= 0 && hoverX < m_gridCols && hoverY >= 0 &&
            hoverY < m_gridRows) {
            auto &dragItem = m_items[dragIdx];
            bool valid =
                canPlace(hoverX, hoverY, dragItem.width, dragItem.height,
                         dragIdx);
            ImU32 hlColor =
                valid ? IM_COL32(0, 200, 0, 50) : IM_COL32(200, 0, 0, 50);

            for (int dy = 0; dy < dragItem.height; dy++) {
                for (int dx = 0; dx < dragItem.width; dx++) {
                    int cx = hoverX + dx;
                    int cy = hoverY + dy;
                    if (cx >= 0 && cx < m_gridCols && cy >= 0 &&
                        cy < m_gridRows) {
                        ImVec2 cellMin(gridOrigin.x + cx * m_slotSize,
                                       gridOrigin.y + cy * m_slotSize);
                        ImVec2 cellMax(cellMin.x + m_slotSize,
                                       cellMin.y + m_slotSize);
                        dl->AddRectFilled(cellMin, cellMax, hlColor);
                    }
                }
            }
        }
    }

    // Draw placed items
    for (int i = 0; i < static_cast<int>(m_items.size()); i++) {
        auto &item = m_items[i];
        if (item.name.empty() || item.gridX < 0 || item.equipped)
            continue;

        ImVec2 itemMin(gridOrigin.x + item.gridX * m_slotSize + 2,
                       gridOrigin.y + item.gridY * m_slotSize + 2);
        ImVec2 itemMax(
            gridOrigin.x + (item.gridX + item.width) * m_slotSize - 2,
            gridOrigin.y + (item.gridY + item.height) * m_slotSize - 2);

        dl->AddRectFilled(itemMin, itemMax, item.color, 4.0f);
        dl->AddRect(itemMin, itemMax, IM_COL32(200, 200, 210, 200), 4.0f, 0,
                    1.0f);

        // Item name
        dl->AddText(ImVec2(itemMin.x + 3, itemMin.y + 2),
                    IM_COL32(255, 255, 255, 255), item.name.c_str());

        // Stack count
        if (item.stackCount > 1) {
            char stackText[16];
            snprintf(stackText, sizeof(stackText), "x%d", item.stackCount);
            ImVec2 stackSize = ImGui::CalcTextSize(stackText);
            dl->AddText(
                ImVec2(itemMax.x - stackSize.x - 3, itemMax.y - stackSize.y - 2),
                IM_COL32(255, 255, 200, 255), stackText);
        }

        // Equippable indicator
        if (item.compatibleSlot != EquipmentSlot::COUNT) {
            const char *slotName = getEquipmentSlotName(item.compatibleSlot);
            ImVec2 slotTextSize = ImGui::CalcTextSize(slotName);
            dl->AddText(
                ImVec2(itemMin.x + 3, itemMax.y - slotTextSize.y - 2),
                IM_COL32(200, 200, 255, 180), slotName);
        }
    }

    // Per-cell invisible buttons for drag-drop interaction
    for (int cy = 0; cy < m_gridRows; cy++) {
        for (int cx = 0; cx < m_gridCols; cx++) {
            ImGui::SetCursorScreenPos(ImVec2(gridOrigin.x + cx * m_slotSize,
                                             gridOrigin.y + cy * m_slotSize));

            char cellId[32];
            snprintf(cellId, sizeof(cellId), "##gc%d_%d", cx, cy);
            ImGui::InvisibleButton(cellId, ImVec2(m_slotSize, m_slotSize));

            int cellVal = m_grid[cy * m_gridCols + cx];

            // Drag source: any cell belonging to an item
            if (cellVal != -1) {
                if (ImGui::BeginDragDropSource(
                        ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("INV_ITEM", &cellVal,
                                              sizeof(int));
                    ImGui::Text("%s", m_items[cellVal].name.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            // Drop target
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload =
                        ImGui::AcceptDragDropPayload("INV_ITEM")) {
                    int droppedIdx = *(const int *)payload->Data;
                    auto &droppedItem = m_items[droppedIdx];

                    // Remove from previous location
                    if (droppedItem.equipped) {
                        for (int s = 0;
                             s < static_cast<int>(EquipmentSlot::COUNT); s++) {
                            if (m_equipped[s] == droppedIdx) {
                                m_equipped[s] = -1;
                                break;
                            }
                        }
                        droppedItem.equipped = false;
                    } else {
                        clearItemFromGrid(droppedIdx);
                    }

                    // Try to place at hovered cell
                    if (canPlace(cx, cy, droppedItem.width, droppedItem.height,
                                 droppedIdx)) {
                        droppedItem.gridX = cx;
                        droppedItem.gridY = cy;
                        markGrid(cx, cy, droppedItem.width, droppedItem.height,
                                 droppedIdx);
                    } else {
                        // Fall back to any free spot
                        int pos = findFirstFreePosition(droppedItem.width,
                                                        droppedItem.height);
                        if (pos >= 0) {
                            droppedItem.gridX = pos % m_gridCols;
                            droppedItem.gridY = pos / m_gridCols;
                            markGrid(droppedItem.gridX, droppedItem.gridY,
                                     droppedItem.width, droppedItem.height,
                                     droppedIdx);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Right-click to equip
            if (cellVal != -1 &&
                ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                auto &item = m_items[cellVal];
                if (item.compatibleSlot != EquipmentSlot::COUNT &&
                    !item.equipped) {
                    equipItem(cellVal);
                }
            }

            // Tooltip
            if (cellVal != -1 &&
                ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                auto &item = m_items[cellVal];
                ImGui::BeginTooltip();
                ImGui::Text("%s", item.name.c_str());
                if (item.compatibleSlot != EquipmentSlot::COUNT) {
                    ImGui::Text("Slot: %s",
                                getEquipmentSlotName(item.compatibleSlot));
                    ImGui::TextDisabled("Right-click to equip");
                }
                if (item.stackCount > 1) {
                    ImGui::Text("Stack: %d / %d", item.stackCount,
                                item.maxStack);
                }
                ImGui::EndTooltip();
            }
        }
    }

    ImGui::EndChild();
}

} // namespace moiras
