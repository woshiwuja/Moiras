#pragma once

#include <sol/sol.hpp>

namespace moiras
{

  class MathBindings
  {
  public:
    static void registerBindings(sol::state &lua);
  };

} // namespace moiras
