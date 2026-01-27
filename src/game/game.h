#pragma once
#include "../../rlImGui/rlImGui.h"
#include "../lights/lightmanager.h"
#include "../window/window.h"
#include "../character/controller.h"
#include "../building/structure_builder.h"
#include "game_object.h"
#include <memory>
#include <raylib.h>
namespace moiras {
class Game {
  RenderTexture2D renderTarget;
  Shader outlineShader;
  float nearPlane;
  float farPlane;
  int depthTextureLoc;

public:
  GameObject root;
  LightManager lightmanager;

  // Player controller
  std::unique_ptr<CharacterController> playerController;

  // Structure builder per map building
  StructureBuilder* structureBuilder = nullptr;

  Game();
  ~Game();
  void setup();
  void loop(Window window);
};
} // namespace moiras
