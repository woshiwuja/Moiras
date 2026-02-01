#pragma once
#include "gui.h"
#include "../../rlImGui/rlImGui.h"
#include <vector>
#include "../game/game_object.h"
#include "../camera/camera.h"
#include "../character/character.h"
namespace moiras
{

    struct InventoryItem
    {
        std::string name;
        int x, y;          // Posizione in alto a sinistra nella griglia
        int width, height; // Dimensioni (es. una spada è 1x3)
        ImColor color;     // Per debug o estetica
    };
    class Inventory : public GameObject
    {
        public:
    Texture2D previewTexture = {0};
    RenderTexture2D previewRenderTarget = {0};
    Camera3D previewCamera = {0};
    ModelManager* m_modelManager = nullptr;
    private:
        int size_x, size_y;
        float slotSize = 15.0f;
        std::vector<char> grid; // Usiamo char per avere riferimenti validi
        std::vector<InventoryItem> items;


    void setupPreviewCamera() {
        previewCamera.position = {3.0f, 3.0f, 3.0f};
        previewCamera.target = {0.0f, 0.5f, 0.0f};
        previewCamera.up = {0.0f, 1.0f, 0.0f};
        previewCamera.fovy = 45.0f;
        previewCamera.projection = CAMERA_PERSPECTIVE;
    }
    public:
        bool isOpen = false;
        Inventory(int w, int h) : size_x(w), size_y(h)
        {
            grid.resize(size_x * size_y, 0);
            setupPreviewCamera();
        }

        // Controlla se un rettangolo di slot è libero
        bool canPlaceItem(int startX, int startY, int w, int h)
        {
            if (startX < 0 || startY < 0 || startX + w > size_x || startY + h > size_y)
                return false;

            for (int yy = startY; yy < startY + h; yy++)
            {
                for (int xx = startX; xx < startX + w; xx++)
                {
                    if (grid[yy * size_x + xx] != 0)
                        return false;
                }
            }
            return true;
        }

        void addItem(InventoryItem item)
        {
            if (canPlaceItem(item.x, item.y, item.width, item.height))
            {
                // Segna gli slot come occupati
                for (int yy = item.y; yy < item.y + item.height; yy++)
                {
                    for (int xx = item.x; xx < item.x + item.width; xx++)
                    {
                        grid[yy * size_x + xx] = 1;
                    }
                }
                items.push_back(item);
            }
        }
        void renderPreview(Model *modelPtr)
        {
            if (!modelPtr)
                return;

            auto renderCamera = getChildOfType<GameCamera>();
            BeginTextureMode(previewRenderTarget);
            ClearBackground(DARKGRAY);
            renderCamera->beginMode3D();

            DrawModel(*modelPtr, {0, 0, 0}, 1.0f, BLACK);

            renderCamera->endMode3D();
            EndTextureMode();
        }
        void update() override;
        void gui() override
        {
            if (IsKeyPressed(KEY_I))
                isOpen = !isOpen;
            if (!isOpen)
                return;
            ImGui::Begin("Inventory", &isOpen);
            ImVec2 previewSize = ImVec2(200, 200);
            if (previewRenderTarget.texture.id > 0)
            {
                auto root = getRoot();
                rlImGuiImageRenderTexture(&previewRenderTarget);
            }

            ImGui::Separator();
            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            for (int i = 0; i <= size_x; i++)
                draw_list->AddLine(ImVec2(canvas_p0.x + i * slotSize, canvas_p0.y),
                                   ImVec2(canvas_p0.x + i * slotSize, canvas_p0.y + size_y * slotSize),
                                   ImColor(150, 150, 150, 50));

            for (int j = 0; j <= size_y; j++)
                draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + j * slotSize),
                                   ImVec2(canvas_p0.x + size_x * slotSize, canvas_p0.y + j * slotSize),
                                   ImColor(150, 150, 150, 50));
            ImGui::InvisibleButton("grid_bin", ImVec2(size_x * slotSize, size_y * slotSize));
            ImGui::End();
        }
    };
}