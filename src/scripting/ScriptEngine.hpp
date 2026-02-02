#pragma once

#include <sol/sol.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace moiras
{

  class GameObject;
  class Game;

  class ScriptEngine
  {
  public:
    static ScriptEngine &instance();

    void initialize();
    void shutdown();

    void hotReload();
    void reloadScript(const std::string &scriptPath);

    sol::state &lua();

    void setScriptsDirectory(const std::filesystem::path &dir);
    void setGameRoot(GameObject *root);
    void setGame(Game *game);

    GameObject *getGameRoot() const;
    Game *getGame() const;

  private:
    ScriptEngine() = default;
    ~ScriptEngine() = default;
    ScriptEngine(const ScriptEngine &) = delete;
    ScriptEngine &operator=(const ScriptEngine &) = delete;

    sol::state m_lua;
    std::filesystem::path m_scriptsDir;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_scriptTimestamps;
    GameObject *m_gameRoot = nullptr;
    Game *m_game = nullptr;
    bool m_initialized = false;
  };

} // namespace moiras
