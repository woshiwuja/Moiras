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

protected:
  vector<unique_ptr<GameObject>> children;
  unsigned int id;
  string name;

public:
  bool isVisible;
  Vector3 position;
  GameObject(const string &name = "");
  GameObject(const GameObject &) = delete;
  GameObject(GameObject &&other) noexcept = default;
  GameObject &operator=(GameObject &&other) noexcept = default;
  GameObject &operator=(const GameObject &) = delete;
  virtual ~GameObject() = default;
  virtual void update();
  virtual void draw();
  virtual void gui();
  void addChild(unique_ptr<GameObject> child);
  GameObject *getChild();
  GameObject *getChildByName(const string &name);

  // Template per ottenere primo figlio di tipo specifico
  template <typename T> T *getChildOfType() {
    for (auto &child : children) {
      if (child) {
        T *casted = dynamic_cast<T *>(child.get());
        if (casted)
          return casted;
      }
    }
    return nullptr;
  }

  // Template per ottenere tutti i figli di tipo specifico
  template <typename T> vector<T *> getChildrenOfType() {
    vector<T *> result;
    for (auto &child : children) {
      if (child) {
        T *casted = dynamic_cast<T *>(child.get());
        if (casted)
          result.push_back(casted);
      }
    }
    return result;
  }

  // Getter/Setter
  string getName() const { return name; }
  void setName(const string &n) { name = n; }
  unsigned int getId() const { return id; }
};
} // namespace moiras
