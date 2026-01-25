#pragma once
#include "../../rlImGui/rlImGui.h"
#include "../lights/lightmanager.h"
#include "../window/window.h"
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
  Game();
  ~Game();
  void setup();
  void loop(Window window);
};
} // namespace moiras
