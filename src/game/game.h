#pragma once
#include "../lights/lightmanager.h"
#include "../resources/model_manager.h"
#include "../window/sdl_window.h"
#include "../rendering/filament_engine.h"
#include "../camera/camera.h"
#include "../character/character.h"
#include "../character/controller.h"
#include "../building/structure_builder.h"
#include "../building/structure.h"
#include "../gui/script_editor.h"
#include "../map/environment.hpp"
#include "game_object.h"
#include <memory>
#include <raymath.h>
namespace moiras
{
  class Game
  {
    // Filament rendering
    SDLWindow m_window;
    FilamentEngine m_filamentEngine;

    // Legacy shaders (will be migrated in later phases)
    // Shader outlineShader;
    // Shader celShader;

    float nearPlane;
    float farPlane;
    std::unordered_map<unsigned int, GameObject *> registry;
    int m_frameCount = 0;

    void updateScriptsRecursive(GameObject *obj, float dt);
    // Shadow casters will be handled by Filament in Phase 3
    // void drawShadowCastersRecursive(GameObject *obj, Material &shadowMat);

  public:
    GameObject root;
    LightManager lightmanager;
    ModelManager modelManager;
    std::unique_ptr<CharacterController> playerController;
    StructureBuilder *structureBuilder = nullptr;
    ScriptEditor *scriptEditor = nullptr;
    bool outlineEnabled = false; // Disabled for Phase 1 (will re-enable in Phase 5)

    Game();
    ~Game();
    void setup();
    void loop();
    void renderLoadingFrame(const char *message, float progress);
    void registerObject(unsigned int id, GameObject *object)
    {
      if (object == nullptr)
      {
        return;
      }
      registry[id] = object;
    };
    template <typename T>
    T *getObjectByID(unsigned int id)
    {
      auto it = registry.find(id);
      if (it != registry.end())
      {
        return dynamic_cast<T *>(it->second);
      }
      return nullptr;
    }

    template <typename T>
    std::vector<T*> getObjectInRange(float radius, const Vector3& position)
    {
      std::vector<T*> results;
      for (const auto &[id, object] : registry)
      {
        auto target = object->position;
        auto distance = abs(sqrt(pow((target.x - position.x), 2) + pow((target.y - position.y), 2) - pow((target.z - position.z), 2)));
        if (radius > distance)
        {
          results.push_back(dynamic_cast<T *>(object));
        }
      }
      return results;
    }
  };
} // namespace moiras
