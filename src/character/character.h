#include "../game/game_object.h"
#include <raylib.h>
#include <string>
namespace moiras {
class Character : public GameObject {
private:
  Quaternion quat_rotation;
public:
  int health;
  std::string name;
  Vector3 eulerRot;
  float scale;
  Model *model;
  Character();
  Character(Model*);
  void update() override;
  virtual void draw() override;
  void gui() override;
};
} // namespace moiras
