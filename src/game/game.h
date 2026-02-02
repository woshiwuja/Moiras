#pragma once
#include "../../rlImGui/rlImGui.h"
#include "../lights/lightmanager.h"
#include "../resources/model_manager.h"
#include "../window/window.h"
#include "../camera/camera.h"
#include "../character/character.h"
#include "../character/controller.h"
#include "../building/structure_builder.h"
#include "game_object.h"
#include <memory>
#include <raylib.h>
namespace moiras
{
  class Game
  {
    RenderTexture2D renderTarget;
    Shader outlineShader;
    Shader celShader;
    float nearPlane;
    float farPlane;
    int depthTextureLoc;
    std::unordered_map<unsigned int, GameObject *> registry;
    int m_frameCount = 0;

    void updateScriptsRecursive(GameObject *obj, float dt);

  public:
    GameObject root;
    LightManager lightmanager;
    ModelManager modelManager;
    std::unique_ptr<CharacterController> playerController;
    StructureBuilder *structureBuilder = nullptr;
    Game();
    ~Game();
    void setup();
    void loop(Window window);
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
