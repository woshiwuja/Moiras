#include "ScriptComponent.hpp"
#include "ScriptEngine.hpp"
#include "../game/game_object.h"
#include <raylib.h>

namespace moiras
{

  ScriptComponent::ScriptComponent(GameObject *owner)
      : m_owner(owner)
  {
  }

  ScriptComponent::~ScriptComponent()
  {
    if (m_loaded && m_onDestroy.valid())
    {
      auto result = m_onDestroy();
      if (!result.valid())
      {
        sol::error err = result;
        TraceLog(LOG_WARNING, "SCRIPTING: Error in on_destroy for '%s': %s",
                 m_scriptPath.c_str(), err.what());
      }
    }
  }

  void ScriptComponent::loadScript(const std::string &scriptPath)
  {
    m_scriptPath = scriptPath;
    m_hasError = false;
    m_lastError.clear();
    m_started = false;

    auto &lua = ScriptEngine::instance().lua();

    // Create isolated environment with access to globals
    m_env = sol::environment(lua, sol::create, lua.globals());

    // Bind self reference
    bindSelfToEnvironment();

    // Load and execute the script file in the environment
    auto result = lua.safe_script_file(scriptPath, m_env, sol::script_pass_on_error);
    if (!result.valid())
    {
      sol::error err = result;
      m_hasError = true;
      m_lastError = err.what();
      TraceLog(LOG_ERROR, "SCRIPTING: Failed to load script '%s': %s",
               scriptPath.c_str(), m_lastError.c_str());
      return;
    }

    cacheFunctions();
    m_loaded = true;

    TraceLog(LOG_INFO, "SCRIPTING: Loaded script '%s' for '%s'",
             scriptPath.c_str(), m_owner->getName().c_str());
  }

  void ScriptComponent::onStart()
  {
    if (!m_loaded || m_hasError || m_started)
      return;
    m_started = true;

    if (!m_onStart.valid())
      return;

    auto result = m_onStart();
    if (!result.valid())
    {
      sol::error err = result;
      handleLuaError("on_start", err);
    }
  }

  void ScriptComponent::onUpdate(float dt)
  {
    if (!m_loaded || m_hasError)
      return;

    if (!m_started)
    {
      onStart();
    }

    if (!m_onUpdate.valid())
      return;

    auto result = m_onUpdate(dt);
    if (!result.valid())
    {
      sol::error err = result;
      handleLuaError("on_update", err);
    }
  }

  void ScriptComponent::onDestroy()
  {
    if (!m_loaded || !m_onDestroy.valid())
      return;

    auto result = m_onDestroy();
    if (!result.valid())
    {
      sol::error err = result;
      handleLuaError("on_destroy", err);
    }
  }

  void ScriptComponent::reload()
  {
    TraceLog(LOG_INFO, "SCRIPTING: Reloading script '%s'", m_scriptPath.c_str());

    // Call on_destroy before reloading
    if (m_loaded && m_onDestroy.valid())
    {
      m_onDestroy();
    }

    // Reset state
    m_loaded = false;
    m_started = false;
    m_hasError = false;
    m_lastError.clear();
    m_onStart = sol::nil;
    m_onUpdate = sol::nil;
    m_onDestroy = sol::nil;

    // Reload
    loadScript(m_scriptPath);
  }

  const std::string &ScriptComponent::getScriptPath() const
  {
    return m_scriptPath;
  }

  bool ScriptComponent::isLoaded() const
  {
    return m_loaded;
  }

  bool ScriptComponent::hasError() const
  {
    return m_hasError;
  }

  const std::string &ScriptComponent::getLastError() const
  {
    return m_lastError;
  }

  void ScriptComponent::bindSelfToEnvironment()
  {
    m_env["self"] = m_owner;
  }

  void ScriptComponent::cacheFunctions()
  {
    m_onStart = m_env["on_start"];
    m_onUpdate = m_env["on_update"];
    m_onDestroy = m_env["on_destroy"];
  }

  void ScriptComponent::handleLuaError(const std::string &context, const sol::error &e)
  {
    m_hasError = true;
    m_lastError = e.what();
    TraceLog(LOG_ERROR, "SCRIPTING: Error in %s for script '%s': %s",
             context.c_str(), m_scriptPath.c_str(), m_lastError.c_str());
  }

} // namespace moiras
