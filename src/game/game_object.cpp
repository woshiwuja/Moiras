#include "game_object.h"
#include "../scripting/ScriptComponent.hpp"
#include <iostream>
#include <memory>

namespace moiras
{

  unsigned int GameObject::nextId = 0;

  GameObject::GameObject(const std::string &name)
      : id(nextId++),
        name(name.empty() ? "GameObject_" + std::to_string(id) : name),
        isVisible(true),
        position(Vector3{0.0f, 0.0f, 0.0f}),
        parent(nullptr)
  {
    std::cout << this->name << "\n";
  }

  // Move constructor
  GameObject::GameObject(GameObject &&other) noexcept
      : m_scriptComponent(std::move(other.m_scriptComponent)),
        children(std::move(other.children)),
        parent(other.parent),
        id(other.id),
        name(std::move(other.name)),
        tag(std::move(other.tag)),
        isVisible(other.isVisible),
        position(other.position)
  {

    // Aggiorna i parent pointer dei children per puntare a questo oggetto
    for (auto &child : children)
    {
      if (child)
      {
        child->parent = this;
      }
    }

    other.parent = nullptr;
  }

  // Move assignment
  GameObject &GameObject::operator=(GameObject &&other) noexcept
  {
    if (this != &other)
    {
      m_scriptComponent = std::move(other.m_scriptComponent);
      children = std::move(other.children);
      parent = other.parent;
      id = other.id;
      name = std::move(other.name);
      tag = std::move(other.tag);
      isVisible = other.isVisible;
      position = other.position;

      for (auto &child : children)
      {
        if (child)
        {
          child->parent = this;
        }
      }
      other.parent = nullptr;
    }
    return *this;
  }

  GameObject::~GameObject() = default;

  void GameObject::update()
  {
    for (size_t i = 0; i < children.size(); ++i)
    {
      if (children[i])
      {
        children[i]->update();
      }
    }
  }

  void GameObject::draw()
  {
    for (const auto &child : children)
    {
      if (child)
      {
        child->draw();
      }
    }
  }

  void GameObject::gui()
  {
    for (const auto &child : children)
    {
      if (child)
      {
        child->gui();
      }
    }
  }

  void GameObject::addChild(std::unique_ptr<GameObject> child)
  {
    if (child)
    {
      child->parent = this; // AGGIUNTO: imposta il parent del child
      std::cout << "Added child '" << child->name << "' to parent '" << this->name << "'\n";
      children.push_back(std::move(child));
    }
  }

  GameObject *GameObject::getChild()
  {
    if (children.empty())
    {
      return nullptr;
    }
    return children[0].get();
  }

  GameObject *GameObject::getChildByName(const std::string &name)
  {
    for (auto &child : children)
    {
      if (child && child->name == name)
      {
        return child.get();
      }
    }
    return nullptr;
  }

  void GameObject::attachScript(const std::string &scriptPath)
  {
    m_scriptComponent = std::make_unique<ScriptComponent>(this);
    m_scriptComponent->loadScript(scriptPath);
  }

} // namespace moiras
