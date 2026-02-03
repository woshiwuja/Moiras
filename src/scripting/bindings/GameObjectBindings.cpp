#include "GameObjectBindings.hpp"
#include "../../game/game_object.h"
#include "../../scripting/ScriptComponent.hpp"
#include <raylib.h>
#include <raymath.h>

namespace moiras
{

  void GameObjectBindings::registerBindings(sol::state &lua)
  {
    // GameObject base type
    lua.new_usertype<GameObject>("GameObject",
                                sol::no_constructor,

                                // Properties
                                "name", sol::property(&GameObject::getName, &GameObject::setName),
                                "id", sol::readonly(&GameObject::id),
                                "position", &GameObject::position,
                                "visible", &GameObject::isVisible,
                                "tag", &GameObject::tag,

                                // Hierarchy
                                "parent", sol::property(&GameObject::getParent),
                                "get_child_by_name", &GameObject::getChildByName,
                                "get_child_count", &GameObject::getChildCount,
                                "get_child_at", &GameObject::getChildAt,
                                "get_root", &GameObject::getRoot,

                                // Script access
                                "has_script", [](GameObject *obj) -> bool
                                { return obj->getScriptComponent() != nullptr; },

                                // Attach a script to this object
                                "attach_script", &GameObject::attachScript);
  }

} // namespace moiras
