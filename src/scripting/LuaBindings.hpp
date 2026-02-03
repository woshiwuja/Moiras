#pragma once

#include <sol/sol.hpp>

namespace moiras
{

  class LuaBindings
  {
  public:
    static void registerAll(sol::state &lua);
  };

} // namespace moiras
