#pragma once

#include "../game/game_object.h"
#include "../navigation/navmesh.h"
#include "structure.h"
#include <raylib.h>
#include <string>
#include <vector>
#include <filesystem>

namespace moiras {

class Map; // Forward declaration

// Gestisce la modalita' building per piazzare strutture
class StructureBuilder : public GameObject {
public:
    StructureBuilder();
    ~StructureBuilder();

    void update() override;
    void draw() override;
    void gui() override;

    // Configura le dipendenze
    void setMap(Map* map);
    void setCamera(Camera3D* camera);
    void setNavMesh(NavMesh* navMesh);

    // Controlla la modalita' building
    bool isBuildingMode() const { return m_buildingMode; }
    void enterBuildingMode();
    void exitBuildingMode();

    // Seleziona un asset da piazzare
    void selectAsset(int index);
    void selectAsset(const std::string& assetPath);

    // Rotazione della preview
    void rotatePreview(float deltaY);

    // Conferma il piazzamento
    bool placeStructure();

    // Lista degli asset disponibili
    const std::vector<std::string>& getAssetList() const { return m_assetFiles; }
    int getSelectedAssetIndex() const { return m_selectedAsset; }
    void refreshAssetList();

    // Preview texture management
    Texture2D getPreviewTexture() const { return m_previewTexture; }

private:
    // Stato
    bool m_buildingMode;
    int m_selectedAsset;
    int m_lastSelectedAsset;
    float m_previewRotationY;
    float m_previewScale;
    bool m_isValidPlacement;

    // Dipendenze
    Map* m_map;
    Camera3D* m_camera;
    NavMesh* m_navMesh;

    // Preview model (mostrato durante il building mode)
    Model m_previewModel;
    bool m_previewModelLoaded;
    Vector3 m_previewPosition;
    Shader m_previewShaderValid;
    Shader m_previewShaderInvalid;
    bool m_previewShadersLoaded;

    // Lista asset
    std::vector<std::string> m_assetFiles;

    // Preview texture per la GUI
    Texture2D m_previewTexture;
    RenderTexture2D m_previewRenderTarget;
    Camera3D m_previewCamera;

    // Stato shader applicato (per evitare applicazioni ripetute)
    bool m_lastShaderWasValid;

    // Metodi privati
    void loadAssetList();
    void loadPreviewModel(const std::string& assetPath);
    void unloadPreviewModel();
    void updatePreviewPosition();
    bool checkPlacementValidity();
    void loadPreviewShaders();
    void unloadPreviewShaders();
    void applyPreviewShader(bool valid);

    // Preview texture generation
    void setupPreviewCamera();
    void loadOrGeneratePreviewTexture(const std::string& assetFilename);
    void generatePreviewTexture(const std::string& assetFilename, const std::string& previewPath);
    std::string getPreviewPath(const std::string& assetFilename);
};

} // namespace moiras
