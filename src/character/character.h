#pragma once

#include "../game/game_object.h"
#include "../resources/model_manager.h"
#include "../map/map.h"
#include <raylib.h>
#include <string>

#include "../gui/inventory.hpp"
namespace moiras {

class Character : public GameObject {
public:
  int health;
  std::string name;
  Vector3 eulerRot;
  bool isVisible = true;
  float scale;
  ModelInstance modelInstance;
  Quaternion quat_rotation;
  std::string model_path="../assets/ogre.glb";

  // Animation data
  ModelAnimation* m_animations = nullptr;
  int m_animationCount = 0;
  int m_currentAnimIndex = -1;
  int m_currentFrame = 0;
  float m_animationTimer = 0.0f;
  bool m_isAnimating = false;

  Character();
  ~Character();

  Character(Character &&other) noexcept;
  Character &operator=(Character &&other) noexcept;

  Character(const Character &) = delete;
  Character &operator=(const Character &) = delete;

  void update() override;
  void draw() override;
  void gui() override;

  void loadModel(ModelManager& manager, const std::string &path);
  void unloadModel();
  void snapToGround(const Model &ground);
  void handleDroppedModel();
  void handleFileDialog();

  // Applica lo shader a tutti i materiali del modello
  void applyShader(Shader shader);

  // Shader statico condiviso da tutti i character
  static Shader sharedShader;
  static void setSharedShader(Shader shader);

  // For backwards compatibility - check if model is loaded
  bool hasModel() const { return modelInstance.isValid(); }

  // Animation methods
  void loadAnimations(const std::string& modelPath);
  void unloadAnimations();
  bool setAnimation(const std::string& animName);
  void playAnimation();
  void stopAnimation();
  void updateAnimation();  // Call every frame to advance animation
  bool isAnimating() const { return m_isAnimating; }
  int getAnimationIndex(const std::string& name) const;
};

} // namespace moiras
