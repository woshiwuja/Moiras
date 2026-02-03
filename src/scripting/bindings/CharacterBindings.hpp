#pragma once

#include <sol/sol.hpp>

namespace moiras
{

  class CharacterBindings
  {
  public:
    static void registerBindings(sol::state &lua);
  };

} // namespace moiras
