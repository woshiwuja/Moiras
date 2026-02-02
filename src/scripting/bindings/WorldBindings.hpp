#pragma once

#include <sol/sol.hpp>

namespace moiras
{

  class WorldBindings
  {
  public:
    static void registerBindings(sol::state &lua);
  };

} // namespace moiras
