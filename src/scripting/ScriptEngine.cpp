#include "ScriptEngine.hpp"
#include "LuaBindings.hpp"
#include "ScriptComponent.hpp"
#include "../game/game_object.h"
#include <raylib.h>

namespace moiras
{

  ScriptEngine &ScriptEngine::instance()
  {
    static ScriptEngine inst;
    return inst;
  }

  void ScriptEngine::initialize()
  {
    if (m_initialized)
      return;

    m_lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::io,
        sol::lib::os,
        sol::lib::package);

    LuaBindings::registerAll(m_lua);

    m_initialized = true;
    TraceLog(LOG_INFO, "SCRIPTING: ScriptEngine initialized");
  }

  void ScriptEngine::shutdown()
  {
    m_scriptTimestamps.clear();
    m_gameRoot = nullptr;
    m_game = nullptr;
    m_initialized = false;
    TraceLog(LOG_INFO, "SCRIPTING: ScriptEngine shut down");
  }

  sol::state &ScriptEngine::lua()
  {
    return m_lua;
  }

  void ScriptEngine::setScriptsDirectory(const std::filesystem::path &dir)
  {
    m_scriptsDir = dir;
  }

  void ScriptEngine::setGameRoot(GameObject *root)
  {
    m_gameRoot = root;
  }

  void ScriptEngine::setGame(Game *game)
  {
    m_game = game;
  }

  GameObject *ScriptEngine::getGameRoot() const
  {
    return m_gameRoot;
  }

  Game *ScriptEngine::getGame() const
  {
    return m_game;
  }

  static void collectScriptComponents(GameObject *obj, std::vector<ScriptComponent *> &out)
  {
    if (!obj)
      return;
    if (auto *sc = obj->getScriptComponent())
    {
      out.push_back(sc);
    }
    for (auto &child : obj->children)
    {
      if (child)
      {
        collectScriptComponents(child.get(), out);
      }
    }
  }

  void ScriptEngine::hotReload()
  {
    if (m_scriptsDir.empty() || !std::filesystem::exists(m_scriptsDir))
      return;

    for (const auto &entry : std::filesystem::directory_iterator(m_scriptsDir))
    {
      if (entry.path().extension() == ".lua")
      {
        auto path = entry.path().string();
        auto lastWrite = std::filesystem::last_write_time(entry);

        auto it = m_scriptTimestamps.find(path);
        if (it == m_scriptTimestamps.end())
        {
          m_scriptTimestamps[path] = lastWrite;
        }
        else if (it->second < lastWrite)
        {
          m_scriptTimestamps[path] = lastWrite;
          reloadScript(path);
        }
      }
    }
  }

  void ScriptEngine::reloadScript(const std::string &scriptPath)
  {
    TraceLog(LOG_INFO, "SCRIPTING: Hot-reloading script: %s", scriptPath.c_str());

    if (!m_gameRoot)
      return;

    std::vector<ScriptComponent *> components;
    collectScriptComponents(m_gameRoot, components);

    for (auto *sc : components)
    {
      if (sc->getScriptPath() == scriptPath)
      {
        sc->reload();
      }
    }
  }

} // namespace moiras
