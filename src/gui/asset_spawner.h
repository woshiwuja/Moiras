#pragma once
#include "../../rlImGui/rlImGui.h"
#include "../game/game_object.h"
#include "../character/character.h"
#include "../resources/model_manager.h"
#include <filesystem>
#include <imgui.h>
#include <raylib.h>
#include <string>
#include <vector>
#include "raymath.h"

namespace moiras {

class AssetSpawner : public GameObject {
private:
    std::vector<std::string> assetFiles;
    int selectedAsset = -1;
    int lastSelectedAsset = -1;
    float spawnPosition[3] = {0.0f, 0.0f, 0.0f};
    float spawnRotation[3] = {0.0f, 0.0f, 0.0f};
    float spawnScale[3] = {1.0f, 1.0f, 1.0f};
    float windowWidth = 600.0f;
    float windowHeight = 350.0f;

    Texture2D previewTexture = {0};
    RenderTexture2D previewRenderTarget = {0};
    Camera3D previewCamera = {0};
    ModelManager* m_modelManager = nullptr;
    
    void loadAssetList() {
        assetFiles.clear();
        std::string assetsPath = "../assets/";
        
        try {
            if (std::filesystem::exists(assetsPath)) {
                for (const auto& entry : std::filesystem::directory_iterator(assetsPath)) {
                    if (entry.is_regular_file()) {
                        std::string ext = entry.path().extension().string();
                        // Convert to lowercase for comparison
                        for (auto& c : ext) c = tolower(c);
                        
                        if (ext == ".glb" || ext == ".obj" || ext == ".fbx" || 
                            ext == ".gltf" || ext == ".blend") {
                            assetFiles.push_back(entry.path().filename().string());
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Handle error - directory doesn't exist or can't be read
        }
    }
    
    void setupPreviewCamera() {
        previewCamera.position = {3.0f, 3.0f, 3.0f};
        previewCamera.target = {0.0f, 0.5f, 0.0f};
        previewCamera.up = {0.0f, 1.0f, 0.0f};
        previewCamera.fovy = 45.0f;
        previewCamera.projection = CAMERA_PERSPECTIVE;
    }
    
    std::string getPreviewPath(const std::string& assetFilename) {
        std::filesystem::path assetPath(assetFilename);
        std::string previewName = assetPath.stem().string() + "-preview.png";
        return "../assets/" + previewName;
    }
    
    void loadOrGeneratePreview(const std::string& assetFilename) {
        // Unload previous texture if exists
        if (previewTexture.id != 0) {
            UnloadTexture(previewTexture);
            previewTexture = {0};
        }
        
        std::string previewPath = getPreviewPath(assetFilename);
        
        // Check if preview already exists
        if (std::filesystem::exists(previewPath)) {
            previewTexture = LoadTexture(previewPath.c_str());
            return;
        }
        
        // Generate new preview
        generatePreview(assetFilename, previewPath);
    }
    
    void generatePreview(const std::string& assetFilename, const std::string& previewPath) {
        // Create render target if not exists
        if (previewRenderTarget.id == 0) {
            previewRenderTarget = LoadRenderTexture(1920, 1080);
            setupPreviewCamera();
        }
        
        // Load the model
        std::string modelPath = "../assets/" + assetFilename;
        Model model = LoadModel(modelPath.c_str());
        
        if (model.meshCount == 0) {
            UnloadModel(model);
            return;
        }
        
        // Calculate model bounds for better camera positioning
        BoundingBox bounds = GetModelBoundingBox(model);
        Vector3 center = {
            (bounds.min.x + bounds.max.x) * 0.5f,
            (bounds.min.y + bounds.max.y) * 0.5f,
            (bounds.min.z + bounds.max.z) * 0.5f
        };
        
        float dimX = bounds.max.x - bounds.min.x;
        float dimY = bounds.max.y - bounds.min.y;
        float dimZ = bounds.max.z - bounds.min.z;
        float maxDim = std::max(std::max(dimX, dimY), dimZ);
        
        float distance = maxDim * 1.2f;
        previewCamera.position = {center.x + distance, center.y + distance, center.z + distance};
        previewCamera.target = center;
        
        // Render to texture
        BeginTextureMode(previewRenderTarget);
            ClearBackground(GRAY);
            BeginMode3D(previewCamera);
                DrawModel(model, {0, 0, 0}, 1.0f, WHITE);
            EndMode3D();
        EndTextureMode();
        
        // Save the preview
        Image img = LoadImageFromTexture(previewRenderTarget.texture);
        ImageFlipVertical(&img);
        ExportImage(img, previewPath.c_str());
        UnloadImage(img);
        
        // Load the saved preview as texture
        previewTexture = LoadTexture(previewPath.c_str());
        
        // Unload the model
        UnloadModel(model);
    }
    
public:
    AssetSpawner() : GameObject("AssetSpawner") {
        loadAssetList();
    }

    void setModelManager(ModelManager* manager) {
        m_modelManager = manager;
    }
    
    ~AssetSpawner() {
        if (previewTexture.id != 0) {
            UnloadTexture(previewTexture);
        }
        if (previewRenderTarget.id != 0) {
            UnloadRenderTexture(previewRenderTarget);
        }
    }
    
    void update() override {
        GameObject::update();
        
        // Check if selection changed
        if (selectedAsset != lastSelectedAsset && selectedAsset >= 0) {
            loadOrGeneratePreview(assetFiles[selectedAsset]);
            lastSelectedAsset = selectedAsset;
        }
    }
    
    void gui() override {
        GameObject::gui();
        
        // Center the window
        ImGui::SetNextWindowPos(
            ImVec2((GetScreenWidth() - windowWidth) * 0.5f, 
                   (GetScreenHeight() - windowHeight) * 0.5f),
            ImGuiCond_FirstUseEver
        );
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("Asset Spawner");
        
        // Split into two columns
        ImGui::BeginChild("LeftPanel", ImVec2(200, 0), true);
        
        // Preview area
        ImVec2 previewSize(180, 120);
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        
        if (previewTexture.id != 0) {
            // Draw the preview texture
            rlImGuiImageRect(&previewTexture, 
                           (int)previewSize.x, (int)previewSize.y,
                           (Rectangle){0, 0, (float)previewTexture.width, (float)previewTexture.height});
        } else {
            // Draw grey placeholder
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + previewSize.x, cursorPos.y + previewSize.y),
                IM_COL32(60, 60, 60, 255)
            );
            ImGui::GetWindowDrawList()->AddRect(
                cursorPos,
                ImVec2(cursorPos.x + previewSize.x, cursorPos.y + previewSize.y),
                IM_COL32(100, 100, 100, 255)
            );
            
            // Center "Preview" text
            ImVec2 textSize = ImGui::CalcTextSize("No Preview");
            ImGui::SetCursorScreenPos(ImVec2(
                cursorPos.x + (previewSize.x - textSize.x) * 0.5f,
                cursorPos.y + (previewSize.y - textSize.y) * 0.5f
            ));
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No Preview");
            
            // Move cursor past the preview
            ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + previewSize.y));
            ImGui::Dummy(ImVec2(previewSize.x, 0));
        }
        
        ImGui::Spacing();
        
        // Asset list
        ImGui::Text("Assets");
        ImGui::BeginChild("AssetList", ImVec2(0, 0), false);
        
        for (size_t i = 0; i < assetFiles.size(); i++) {
            if (ImGui::Selectable(assetFiles[i].c_str(), selectedAsset == (int)i)) {
                selectedAsset = i;
            }
        }
        
        ImGui::EndChild();
        
        ImGui::EndChild();
        
        // Right panel - controls
        ImGui::SameLine();
        ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
        
        // Transform controls
        ImGui::Text("Position");
        ImGui::DragFloat("X##pos", &spawnPosition[0], 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat("Y##pos", &spawnPosition[1], 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat("Z##pos", &spawnPosition[2], 0.1f, -100.0f, 100.0f);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Rotation");
        ImGui::SliderFloat("X##rot", &spawnRotation[0], 0.0f, 360.0f, "%.0f°");
        ImGui::SliderFloat("Y##rot", &spawnRotation[1], 0.0f, 360.0f, "%.0f°");
        ImGui::SliderFloat("Z##rot", &spawnRotation[2], 0.0f, 360.0f, "%.0f°");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Scale");
        ImGui::DragFloat("X##scale", &spawnScale[0], 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Y##scale", &spawnScale[1], 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Z##scale", &spawnScale[2], 0.01f, 0.01f, 10.0f);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Buttons at the bottom
        if (ImGui::Button("Refresh List", ImVec2(-1, 0))) {
            loadAssetList();
        }
        
        if (ImGui::Button("Regenerate Preview", ImVec2(-1, 0))) {
            if (selectedAsset >= 0) {
                std::string previewPath = getPreviewPath(assetFiles[selectedAsset]);
                if (std::filesystem::exists(previewPath)) {
                    std::filesystem::remove(previewPath);
                }
                generatePreview(assetFiles[selectedAsset], previewPath);
            }
        }
        
        if (ImGui::Button("Spawn Asset", ImVec2(-1, 30))) {
            if (selectedAsset >= 0) {
                spawnAsset();
            }
        }
        
        ImGui::EndChild();
        
        ImGui::End();
    }
    
    std::string getSelectedAsset() const {
        if (selectedAsset >= 0 && selectedAsset < (int)assetFiles.size()) {
            return assetFiles[selectedAsset];
        }
        return "";
    }
    
    void getSpawnPosition(float& x, float& y, float& z) const {
        x = spawnPosition[0];
        y = spawnPosition[1];
        z = spawnPosition[2];
    }
    
    void getSpawnRotation(float& x, float& y, float& z) const {
        x = spawnRotation[0];
        y = spawnRotation[1];
        z = spawnRotation[2];
    }
    
    void getSpawnScale(float& x, float& y, float& z) const {
        x = spawnScale[0];
        y = spawnScale[1];
        z = spawnScale[2];
    }
    
    void spawnAsset() {
        if (selectedAsset < 0 || selectedAsset >= (int)assetFiles.size()) {
            return;
        }
        if (!m_modelManager) {
            TraceLog(LOG_ERROR, "AssetSpawner: ModelManager not set!");
            return;
        }

        // Create a new Character
        auto character = std::make_unique<Character>();

        // Load the model
        std::string modelPath = "../assets/" + assetFiles[selectedAsset];
        character->loadModel(*m_modelManager, modelPath);

        // Set position
        character->position = {spawnPosition[0], spawnPosition[1], spawnPosition[2]};

        // Set rotation
        character->eulerRot = {
            spawnRotation[0] * DEG2RAD,
            spawnRotation[1] * DEG2RAD,
            spawnRotation[2] * DEG2RAD
        };

        // Set scale
        character->scale = spawnScale[0];

        // Set name
        std::filesystem::path assetPath(assetFiles[selectedAsset]);
        character->name = assetPath.stem().string();

        // Snap to ground
        GameObject* root = getRoot();
        if (root) {
            // Find the Map in the scene
            auto map = root->getChildOfType<Map>();
            if (map) {
                character->snapToGround(map->model);
            }

            // Add to scene
            root->addChild(std::move(character));
        }
    }
};

} // namespace moiras
