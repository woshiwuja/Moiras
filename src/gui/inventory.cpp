#include "inventory.hpp"
#include "../game/game.h"
namespace moiras
{
    void moiras::Inventory::update()
    {
        if (!isOpen)
            return;

        auto parent = dynamic_cast<Character *>(getParent());
        auto camera = getChildOfType<GameCamera>();
        if (parent && parent->hasModel())
        {
            camera->update();
            previewRenderTarget = LoadRenderTexture(1920, 1080);
            Model tempModel = {0};
            tempModel.meshCount = parent->modelInstance.meshCount();
            tempModel.meshes = parent->modelInstance.meshes();
            tempModel.materialCount = parent->modelInstance.materialCount();
            tempModel.materials = parent->modelInstance.materials();
            tempModel.meshMaterial = parent->modelInstance.meshMaterial();
            renderPreview(&tempModel);
        }
    };
        
}