#define RAYGUI_IMPLEMENTATION
#include "camera/camera.h"
#include "character/character.h"
#include "game/game.h"
#include "raylib.h"
#include "rcamera.h"

const int screenWidth = 1920;
const int screenHeight = 1080;
string title = "Moiras";

using namespace moiras;
int main(void) {
  Game game = Game();
  Window window = Window(screenWidth, screenHeight, title, KEY_END, 144, true);
  window.init();
  game.setup();
  game.loop(window);
  return 0;
}
