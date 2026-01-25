#pragma once

#include "../game/game_object.h"
#include <raylib.h>
#include <string>

namespace moiras {

enum class LightType { DIRECTIONAL = 0, POINT = 1, SPOT = 2 };

class Light : public GameObject {
public:
  bool enabled = true;
  Vector3 target = {0, 0, 0};
  Color color = WHITE;
  float intensity = 1.0f;
  // Shader locations
  int typeLoc = -1;
  int enabledLoc = -1;
  int positionLoc = -1;
  int targetLoc = -1;
  int colorLoc = -1;
  int intensityLoc = -1;

  Light(const std::string &name = "Light");
  virtual ~Light() = default;

  virtual LightType getType() const = 0;

  // Inizializza le location dello shader
  void setupShaderLocations(Shader shader, int lightIndex);

  // Aggiorna i valori nello shader
  void updateShader(Shader shader);

  void update() override;
  void draw() override;
  void gui() override;
  void guiControl();

protected:
  float colorNormalized[4];
  void normalizeColor();
};

class PointLight : public Light {
public:
  PointLight(const std::string &name = "PointLight");
  LightType getType() const override { return LightType::POINT; }
  void draw() override;
};

class SpotLight : public Light {
public:
  SpotLight(const std::string &name = "SpotLight");
  LightType getType() const override { return LightType::SPOT; }
  void draw() override;
};

class DirectionalLight : public Light {
public:
  DirectionalLight(const std::string &name = "DirectionalLight");
  LightType getType() const override { return LightType::DIRECTIONAL; }
  void draw() override;
};

} // namespace moiras
