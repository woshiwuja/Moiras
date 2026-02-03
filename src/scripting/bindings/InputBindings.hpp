#pragma once

#include <sol/sol.hpp>

namespace moiras
{

  class InputBindings
  {
  public:
    static void registerBindings(sol::state &lua);
  };

} // namespace moiras
