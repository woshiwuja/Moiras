#include "WorldBindings.hpp"
#include "../ScriptEngine.hpp"
#include "../../game/game_object.h"
#include <raylib.h>
#include <string>
#include <vector>

namespace moiras
{

  static GameObject *findByNameRecursive(GameObject *root, const std::string &name)
  {
    if (!root)
      return nullptr;
    if (root->getName() == name)
      return root;
    for (auto &child : root->children)
    {
      if (child)
      {
        auto *found = findByNameRecursive(child.get(), name);
        if (found)
          return found;
      }
    }
    return nullptr;
  }

  static void findAllByTagRecursive(GameObject *root, const std::string &tagName,
                                    std::vector<GameObject *> &out)
  {
    if (!root)
      return;
    if (root->tag == tagName)
      out.push_back(root);
    for (auto &child : root->children)
    {
      if (child)
      {
        findAllByTagRecursive(child.get(), tagName, out);
      }
    }
  }

  static void findAllByNameRecursive(GameObject *root, const std::string &name,
                                     std::vector<GameObject *> &out)
  {
    if (!root)
      return;
    if (root->getName() == name)
      out.push_back(root);
    for (auto &child : root->children)
    {
      if (child)
      {
        findAllByNameRecursive(child.get(), name, out);
      }
    }
  }

  static GameObject *findByIdRecursive(GameObject *root, unsigned int id)
  {
    if (!root)
      return nullptr;
    if (root->getId() == id)
      return root;
    for (auto &child : root->children)
    {
      if (child)
      {
        auto *found = findByIdRecursive(child.get(), id);
        if (found)
          return found;
      }
    }
    return nullptr;
  }

  void WorldBindings::registerBindings(sol::state &lua)
  {
    auto world = lua.create_named_table("World");

    // Find single object by name
    world["find_by_name"] = [](const std::string &name) -> GameObject *
    {
      auto *root = ScriptEngine::instance().getGameRoot();
      return findByNameRecursive(root, name);
    };

    // Find all objects by name
    world["find_all_by_name"] = [](const std::string &name) -> std::vector<GameObject *>
    {
      auto *root = ScriptEngine::instance().getGameRoot();
      std::vector<GameObject *> results;
      findAllByNameRecursive(root, name, results);
      return results;
    };

    // Find single object by ID (traverses scene tree)
    world["find_by_id"] = [](unsigned int id) -> GameObject *
    {
      auto *root = ScriptEngine::instance().getGameRoot();
      return findByIdRecursive(root, id);
    };

    // Find all objects by tag
    world["find_all_by_tag"] = [](const std::string &tagName) -> std::vector<GameObject *>
    {
      auto *root = ScriptEngine::instance().getGameRoot();
      std::vector<GameObject *> results;
      findAllByTagRecursive(root, tagName, results);
      return results;
    };

    // Utility functions
    world["get_frame_time"] = []()
    { return GetFrameTime(); };
    world["get_time"] = []()
    { return GetTime(); };
    world["get_fps"] = []()
    { return GetFPS(); };

    // Override print to route through raylib's TraceLog
    lua.set_function("print", [](sol::variadic_args va, sol::this_state ts)
                     {
      sol::state_view sv(ts);
      std::string output;
      for (size_t i = 0; i < va.size(); ++i)
      {
        if (i > 0) output += "\t";
        // Use Lua's tostring for proper conversion
        sol::function tostr = sv["tostring"];
        if (tostr.valid()) {
          sol::object result = tostr(va[i]);
          output += result.as<std::string>();
        }
      }
      TraceLog(LOG_INFO, "LUA: %s", output.c_str()); });

    // Log functions at different levels
    lua.set_function("log_info", [](const std::string &msg)
                     { TraceLog(LOG_INFO, "LUA: %s", msg.c_str()); });
    lua.set_function("log_warning", [](const std::string &msg)
                     { TraceLog(LOG_WARNING, "LUA: %s", msg.c_str()); });
    lua.set_function("log_error", [](const std::string &msg)
                     { TraceLog(LOG_ERROR, "LUA: %s", msg.c_str()); });
  }

} // namespace moiras
