#pragma once

#include <sol/sol.hpp>
#include <string>

namespace moiras
{

  class GameObject;

  class ScriptComponent
  {
  public:
    explicit ScriptComponent(GameObject *owner);
    ~ScriptComponent();

    void loadScript(const std::string &scriptPath);

    void onStart();
    void onUpdate(float dt);
    void onDestroy();

    void reload();

    const std::string &getScriptPath() const;
    bool isLoaded() const;
    bool hasError() const;
    const std::string &getLastError() const;

  private:
    GameObject *m_owner;
    std::string m_scriptPath;

    sol::environment m_env;

    sol::safe_function m_onStart;
    sol::safe_function m_onUpdate;
    sol::safe_function m_onDestroy;

    bool m_started = false;
    bool m_loaded = false;
    bool m_hasError = false;
    std::string m_lastError;

    void bindSelfToEnvironment();
    void cacheFunctions();
    void handleLuaError(const std::string &context, const sol::error &e);
  };

} // namespace moiras
