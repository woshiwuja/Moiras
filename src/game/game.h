#pragma once
#include "../../rlImGui/rlImGui.h"
#include "../lights/lightmanager.h"
#include "../resources/model_manager.h"
#include "../window/window.h"
#include "../camera/camera.h"
#include "../character/character.h"
#include "../character/controller.h"
#include "../building/structure_builder.h"
#include "../building/structure.h"
#include "../gui/script_editor.h"
#include "../map/environment.hpp"
#include "game_object.h"
#include <memory>
#include <raylib.h>
#include <r3d/r3d.h>

namespace moiras
{
  class Game
  {
    R3D_Light r3dDirLight  = 0;
    R3D_Light r3dPointLight = 0;
    std::unordered_map<unsigned int, GameObject *> registry;
    int m_frameCount = 0;

    void updateScriptsRecursive(GameObject *obj, float dt);

  public:
    GameObject root;
    LightManager lightmanager;
    ModelManager modelManager;
    std::unique_ptr<CharacterController> playerController;
    StructureBuilder *structureBuilder = nullptr;
    ScriptEditor *scriptEditor = nullptr;

    Game();
    ~Game();
    void setup();
    void loop(Window window);
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
    std::vector<T*> getObjectInRange(float radius, Vector3 position)
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
