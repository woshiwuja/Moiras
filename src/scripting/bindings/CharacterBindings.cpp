#include "CharacterBindings.hpp"
#include "../../game/game_object.h"
#include "../../character/character.h"
#include <raylib.h>

namespace moiras
{

  void CharacterBindings::registerBindings(sol::state &lua)
  {
    // Character type (extends GameObject)
    lua.new_usertype<Character>("Character",
                                sol::no_constructor,
                                sol::base_classes, sol::bases<GameObject>(),

                                // Character-specific properties
                                "health", &Character::health,
                                "scale", &Character::scale,
                                "euler_rotation", &Character::eulerRot,
                                "quaternion_rotation", &Character::quat_rotation,

                                // Animation
                                "is_animating", &Character::isAnimating,
                                "play_animation", &Character::playAnimation,
                                "stop_animation", &Character::stopAnimation,
                                "set_animation", &Character::setAnimation);
  }

} // namespace moiras
