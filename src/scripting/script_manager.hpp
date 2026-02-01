#include "../game/game_object.h"
#include <lua.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
namespace moiras
{
    class ScriptManager
    {
        lua_State *L = lua_open();
        ScriptManager()
        {
            luaL_openlibs(L);
        };
        ~ScriptManager();
    };
    class Script : public GameObject
    {
        const std::string path;
        void loadScript();
    };
}