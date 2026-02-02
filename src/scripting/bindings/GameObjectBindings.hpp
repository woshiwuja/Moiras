#pragma once

#include <sol/sol.hpp>

namespace moiras
{

  class GameObjectBindings
  {
  public:
    static void registerBindings(sol::state &lua);
  };

} // namespace moiras
