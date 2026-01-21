#include "game_object.h"
#include <iostream>
#include <memory>
namespace moiras {

unsigned int GameObject::nextId = 0;

GameObject::GameObject(const std::string &name)
    : id(nextId++),
      name(name.empty() ? "GameObject_" + std::to_string(id) : name),
      isVisible(true), position(Vector3{0.0f, 0.0f, 0.0f}) {
  std::cout << this->name << "\n";
};

void GameObject::update() {
  for (auto &child : children) {
    child->update();
  }
}
void GameObject::draw() {
  for (auto &child : children) {
    child->draw();
  }
}
void GameObject::gui() {
  for (auto &child : children) {
    child->gui();
  }
}

void GameObject::addChild(std::unique_ptr<GameObject> child) {
  children.push_back(std::move(child));
};

GameObject *GameObject::getChild() {
  if (children.empty()) {
    return nullptr;
  }
  return children[0].get();
}

GameObject *GameObject::getChildByName(const std::string &name) {
  for (auto &child : children) {
    if (child && child->name == name) {
      return child.get();
    }
  }
  return nullptr;
};
} // namespace moiras
