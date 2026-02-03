#include "LuaBindings.hpp"
#include "bindings/MathBindings.hpp"
#include "bindings/InputBindings.hpp"
#include "bindings/WorldBindings.hpp"
#include "bindings/GameObjectBindings.hpp"
#include "bindings/CharacterBindings.hpp"

namespace moiras
{

  void LuaBindings::registerAll(sol::state &lua)
  {
    MathBindings::registerBindings(lua);
    InputBindings::registerBindings(lua);
    GameObjectBindings::registerBindings(lua);
    CharacterBindings::registerBindings(lua);
    WorldBindings::registerBindings(lua);
  }

} // namespace moiras
