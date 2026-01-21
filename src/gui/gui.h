#include "../../raygui/src/raygui.h"
#include "../game/game_object.h"
namespace moiras {
class Gui : GameObject {
public:
  Gui();
  virtual void update() override;
  virtual void draw() override;
};
} // namespace moiras
