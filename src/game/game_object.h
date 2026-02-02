#pragma once
#include <memory>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>
using namespace std;

namespace moiras
{
  class ScriptComponent;

  class GameObject
  {
  private:
    static unsigned int nextId;
    std::unique_ptr<ScriptComponent> m_scriptComponent;

  public:
      vector<unique_ptr<GameObject>> children;
    unsigned int id;
    string name;
    string tag;
    bool isVisible;
    Vector3 position;
    GameObject *parent; // CAMBIATO: raw pointer invece di unique_ptr
    GameObject(const string &name = "");
    GameObject(const GameObject &) = delete;
    GameObject(GameObject &&other) noexcept;
    GameObject &operator=(GameObject &&other) noexcept;
    GameObject &operator=(const GameObject &) = delete;
    virtual ~GameObject();

    virtual void update();
    virtual void draw();
    virtual void gui();

    void addChild(unique_ptr<GameObject> child);
    GameObject *getChild();
    GameObject *getChildByName(const string &name);
    size_t getChildCount() const { return children.size(); }
    GameObject *getChildAt(size_t index)
    {
      if (index < children.size())
      {
        return children[index].get();
      }
      return nullptr;
    }
    // Template per ottenere primo figlio di tipo specifico
    template <typename T>
    T *getChildOfType()
    {
      for (auto &child : children)
      {
        if (child)
        {
          T *casted = dynamic_cast<T *>(child.get());
          if (casted)
            return casted;
        }
      }
      return nullptr;
    }

    // Template per ottenere tutti i figli di tipo specifico
    template <typename T>
    vector<T *> getChildrenOfType()
    {
      vector<T *> result;
      for (auto &child : children)
      {
        if (child)
        {
          T *casted = dynamic_cast<T *>(child.get());
          if (casted)
            result.push_back(casted);
        }
      }
      return result;
    }

    GameObject *getParent()
    {
      return parent; // CAMBIATO: ritorna direttamente il raw pointer
    }

    // Getter/Setter
    string getName() const { return name; }
    const char *getNameC() const
    {
      return name.c_str();
    } // CORRETTO: ritorna const char*
    void setName(const string &n) { name = n; }
    string getTag() const { return tag; }
    void setTag(const string &t) { tag = t; }
    unsigned int getId() const { return id; }
    GameObject *getRoot()
    {
      GameObject *current = this;
      while (current->parent != nullptr)
      {
        current = current->parent;
      }
      return current;
    }

    // Scripting
    void attachScript(const std::string &scriptPath);
    ScriptComponent *getScriptComponent() { return m_scriptComponent.get(); }
    const ScriptComponent *getScriptComponent() const { return m_scriptComponent.get(); }
  };
  // Const version if needed
} // namespace moiras
