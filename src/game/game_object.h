#pragma once
#include "../../raygui/src/raygui.h"
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

public:
  string name;
  bool isVisible;
  Vector3 position;
  GameObject *parent; // CAMBIATO: raw pointer invece di unique_ptr
  GameObject(const string &name = "");
  GameObject(const GameObject &) = delete;
  GameObject(GameObject &&other) noexcept;
  GameObject &operator=(GameObject &&other) noexcept;
  GameObject &operator=(const GameObject &) = delete;
  virtual ~GameObject() = default;

  virtual void update();
  virtual void draw();
  virtual void gui();

  void addChild(unique_ptr<GameObject> child);
  GameObject *getChild();
  GameObject *getChildByName(const string &name);
  size_t getChildCount() const { return children.size(); }
  GameObject *getChildAt(size_t index) {
    if (index < children.size()) {
      return children[index].get();
    }
    return nullptr;
  }
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

  GameObject *getParent() {
    return parent; // CAMBIATO: ritorna direttamente il raw pointer
  }

  // Getter/Setter
  string getName() const { return name; }
  const char *getNameC() const {
    return name.c_str();
  } // CORRETTO: ritorna const char*
  void setName(const string &n) { name = n; }
  unsigned int getId() const { return id; }
  GameObject *getRoot() {
    GameObject *current = this;
    while (current->parent != nullptr) {
      current = current->parent;
    }
    return current;
  }
};
// Const version if needed
} // namespace moiras
