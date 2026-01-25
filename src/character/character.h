#pragma once

#include "../game/game_object.h"
#include "../map/map.h"
#include <raylib.h>
#include <string>

namespace moiras {

class Character : public GameObject {
public:
  int health;
  std::string name;
  Vector3 eulerRot;
  bool isVisible = true;
  float scale;
  Model model;
  Quaternion quat_rotation;

  Character();
  Character(Model *);
  ~Character();

  Character(Character &&other) noexcept;
  Character &operator=(Character &&other) noexcept;

  Character(const Character &) = delete;
  Character &operator=(const Character &) = delete;

  void update() override;
  void draw() override;
  void gui() override;

  void loadModel(const std::string &path);
  void unloadModel();
  void snapToGround(const Model &ground);
  void handleDroppedModel();
  void handleFileDialog();

  // Applica lo shader a tutti i materiali del modello
  void applyShader(Shader shader);

  // Shader statico condiviso da tutti i character
  static Shader sharedShader;
  static void setSharedShader(Shader shader);
};

} // namespace moiras
