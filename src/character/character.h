#include "../game/game_object.h"
#include "../map/map.h"
#include <raylib.h>
#include <string>
namespace moiras {
class Character : public GameObject {
private:
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
  ~Character(); // AGGIUNGI distruttore

  // AGGIUNGI move constructor e assignment
  Character(Character &&other) noexcept;
  Character &operator=(Character &&other) noexcept;

  // DISABILITA copy constructor e assignment
  Character(const Character &) = delete;
  Character &operator=(const Character &) = delete;

  void update() override;
  virtual void draw() override;
  void gui() override;
  void loadModel(const std::string &path);
  void unloadModel();
  void snapToGround(const Model &ground);
  void handleDroppedModel();
  void handleFileDialog();
};
} // namespace moiras
