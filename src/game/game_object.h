#pragma once
#include <memory>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>
using namespace std;
namespace moiras {
class GameObject {
private:
  static unsigned int nextId;

public:
  unsigned int id;
  string name;
  vector<unique_ptr<GameObject>> children;
  bool isVisible;
  Vector3 position;
  GameObject(const string &name = "");
  virtual ~GameObject() = default;
  virtual void update();
  virtual void draw();
  void addChild(unique_ptr<GameObject> child);
};
} // namespace moiras
