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
        dist = (modelHeight * 0.9f) /
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

    Matrix transform =
        MatrixScale(parent->scale, parent->scale, parent->scale);

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
        return false;

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
    // Note: Character::update() doesn't propagate to children, so the
    // preview rendering is done inside gui() instead.
    GameObject::update();
}

void Inventory::gui() {
    if (IsKeyPressed(KEY_I))
        isOpen = !isOpen;
    if (!isOpen)
        return;

    // Render the 3D preview to texture BEFORE the ImGui window.
    // This is safe: ImGui is only building command lists at this point,
    // the actual ImGui draw happens later in rlImGuiEnd().
    renderCharacterPreview();

    // Compute window size from grid + right panel
    float gridW = m_gridCols * m_slotSize + 24.0f; // grid + child padding
    float rightW = static_cast<float>(PREVIEW_W) + 24.0f;
    float winW = gridW + rightW + 16.0f;
    float winH = static_cast<float>(PREVIEW_H) + 400.0f;

    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::Begin("Inventory", &isOpen,
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize);

    // ---- Left: grid inventory ----
    ImGui::BeginChild("LeftPanel", ImVec2(gridW, 0), ImGuiChildFlags_None);
    drawGridZone();
    ImGui::EndChild();

    ImGui::SameLine();

    // ---- Right: preview on top, equipment below ----
    ImGui::BeginChild("RightPanel", ImVec2(0, 0), ImGuiChildFlags_None);
    drawPreviewZone();
    ImGui::Spacing();
    drawEquipmentZone();
    ImGui::EndChild();

    ImGui::End();
}

// ---------------------------------------------------------------------------
// Preview zone
// ---------------------------------------------------------------------------

void Inventory::drawPreviewZone() {
    ImGui::BeginChild(
        "PreviewZone",
        ImVec2(0, static_cast<float>(PREVIEW_H) + 16.0f),
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
// Equipment zone  (grid-based paper-doll layout)
// ---------------------------------------------------------------------------

void Inventory::drawEquipmentZone() {
    ImGui::BeginChild(
        "EquipmentZone",
        ImVec2(0, 0),
        ImGuiChildFlags_Borders);

    ImGui::TextUnformatted("Equipment");
    ImGui::Separator();
    ImGui::Spacing();

    float es = m_equipSlotSize;
    float gap = 4.0f;
    ImVec2 origin = ImGui::GetCursorScreenPos();

    // Horizontal layout: [L.Hand 1x3] [gap] [Body 2xN] [gap] [R.Hand 1x3]
    float handW = 1.0f * es;
    float bodyW = 2.0f * es;
    float bodyX = handW + gap;
    float rHandX = bodyX + bodyW + gap;

    // Vertical layout with gaps between each piece
    float headY = 0.0f;
    float chestY = 2.0f * es + gap;
    float legsY = chestY + 3.0f * es + gap;
    float feetY = legsY + 2.0f * es + gap;

    // Draw all equipment slots as mini-grids
    drawEquipSlotGrid("Head", EquipmentSlot::Head, 2, 2,
                      ImVec2(origin.x + bodyX, origin.y + headY));
    drawEquipSlotGrid("Chest", EquipmentSlot::Chest, 2, 3,
                      ImVec2(origin.x + bodyX, origin.y + chestY));
    drawEquipSlotGrid("L.Hand", EquipmentSlot::LeftHand, 1, 3,
                      ImVec2(origin.x, origin.y + chestY));
    drawEquipSlotGrid("R.Hand", EquipmentSlot::RightHand, 1, 3,
                      ImVec2(origin.x + rHandX, origin.y + chestY));
    drawEquipSlotGrid("Legs", EquipmentSlot::Legs, 2, 2,
                      ImVec2(origin.x + bodyX, origin.y + legsY));
    drawEquipSlotGrid("Feet", EquipmentSlot::Feet, 2, 2,
                      ImVec2(origin.x + bodyX, origin.y + feetY));

    // Reserve space so ImGui scrolling / layout knows the content size
    float totalW = rHandX + handW;
    float totalH = feetY + 2.0f * es;
    ImGui::Dummy(ImVec2(totalW, totalH));

    ImGui::EndChild();
}

void Inventory::drawEquipSlotGrid(const char *label, EquipmentSlot slot,
                                  int slotW, int slotH, ImVec2 pos) {
    int slotIdx = static_cast<int>(slot);
    int itemIdx = m_equipped[slotIdx];
    float es = m_equipSlotSize;
    float totalW = slotW * es;
    float totalH = slotH * es;

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImGui::PushID(slotIdx + 100);

    // --- Background ---
    dl->AddRectFilled(pos, ImVec2(pos.x + totalW, pos.y + totalH),
                      IM_COL32(30, 30, 35, 255), 3.0f);

    // --- Grid lines ---
    for (int x = 0; x <= slotW; x++) {
        float px = pos.x + x * es;
        dl->AddLine(ImVec2(px, pos.y), ImVec2(px, pos.y + totalH),
                    IM_COL32(60, 60, 70, 255));
    }
    for (int y = 0; y <= slotH; y++) {
        float py = pos.y + y * es;
        dl->AddLine(ImVec2(pos.x, py), ImVec2(pos.x + totalW, py),
                    IM_COL32(60, 60, 70, 255));
    }

    // --- Item fill or label ---
    if (itemIdx != -1) {
        auto &item = m_items[itemIdx];
        dl->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2),
                          ImVec2(pos.x + totalW - 2, pos.y + totalH - 2),
                          item.color, 3.0f);
        dl->AddRect(ImVec2(pos.x + 2, pos.y + 2),
                    ImVec2(pos.x + totalW - 2, pos.y + totalH - 2),
                    IM_COL32(220, 200, 100, 200), 3.0f, 0, 1.5f);
        dl->AddText(ImVec2(pos.x + 4, pos.y + 3),
                    IM_COL32(255, 255, 255, 255), item.name.c_str());
    } else {
        ImVec2 textSize = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(pos.x + (totalW - textSize.x) * 0.5f,
                           pos.y + (totalH - textSize.y) * 0.5f),
                    IM_COL32(80, 80, 100, 255), label);
    }

    // --- Invisible button for interaction ---
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("##eslot", ImVec2(totalW, totalH));

    // --- Drag highlight when hovering with a compatible item ---
    const ImGuiPayload *dragPayload = ImGui::GetDragDropPayload();
    if (dragPayload && dragPayload->IsDataType("INV_ITEM") &&
        dragPayload->Data && ImGui::IsItemHovered()) {
        int dIdx = *(const int *)dragPayload->Data;
        if (dIdx >= 0 && dIdx < static_cast<int>(m_items.size())) {
            bool compat = (m_items[dIdx].compatibleSlot == slot);
            ImU32 hlColor =
                compat ? IM_COL32(0, 200, 0, 50) : IM_COL32(200, 0, 0, 50);
            dl->AddRectFilled(pos, ImVec2(pos.x + totalW, pos.y + totalH),
                              hlColor);
        }
    }

    // --- Drag source ---
    if (itemIdx != -1 &&
        ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("INV_ITEM", &itemIdx, sizeof(int));
        ImGui::Text("%s", m_items[itemIdx].name.c_str());
        ImGui::EndDragDropSource();
    }

    // --- Drop target ---
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
                    for (int i = 0;
                         i < static_cast<int>(EquipmentSlot::COUNT); i++) {
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

    // --- Right-click to unequip ---
    if (itemIdx != -1 && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        unequipItem(slot);
    }

    // --- Tooltip ---
    if (itemIdx != -1 && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", m_items[itemIdx].name.c_str());
        ImGui::TextDisabled("Right-click to unequip");
        ImGui::EndTooltip();
    } else if (itemIdx == -1 &&
               ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::Text("%s slot", label);
        ImGui::EndTooltip();
    }

    ImGui::PopID();
}

// ---------------------------------------------------------------------------
// Grid zone (backpack)
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
        dl->AddLine(ImVec2(px, gridOrigin.y),
                    ImVec2(px, gridOrigin.y + gridH),
                    IM_COL32(60, 60, 70, 255));
    }
    for (int y = 0; y <= m_gridRows; y++) {
        float py = gridOrigin.y + y * m_slotSize;
        dl->AddLine(ImVec2(gridOrigin.x, py),
                    ImVec2(gridOrigin.x + gridW, py),
                    IM_COL32(60, 60, 70, 255));
    }

    // Drag highlight
    const ImGuiPayload *dragPayload = ImGui::GetDragDropPayload();
    bool isDragging = dragPayload && dragPayload->IsDataType("INV_ITEM") &&
                      dragPayload->Data;
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
            bool valid = canPlace(hoverX, hoverY, dragItem.width,
                                  dragItem.height, dragIdx);
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
            dl->AddText(ImVec2(itemMax.x - stackSize.x - 3,
                               itemMax.y - stackSize.y - 2),
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

            // Drag source
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
