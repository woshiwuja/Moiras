#include <raylib.h>
#include <string>
struct Quat {
  float w, x, y, z;
};
struct XVector3 {
  float x, y, z;
  XVector3 normalize();
};
struct Euler {
  float x, y, z;
};
class Character {
public:
  int health;
  std::string name;
  XVector3 position;
  Euler eulerRot;
  float scale;
  Model *model;
};
