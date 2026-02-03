#include "InputBindings.hpp"
#include <raylib.h>

namespace moiras
{

  void InputBindings::registerBindings(sol::state &lua)
  {
    auto input = lua.create_named_table("Input");

    // Keyboard
    input["is_key_down"] = [](int key)
    { return IsKeyDown(key); };
    input["is_key_pressed"] = [](int key)
    { return IsKeyPressed(key); };
    input["is_key_released"] = [](int key)
    { return IsKeyReleased(key); };
    input["is_key_up"] = [](int key)
    { return IsKeyUp(key); };

    // Mouse buttons
    input["is_mouse_button_down"] = [](int button)
    { return IsMouseButtonDown(button); };
    input["is_mouse_button_pressed"] = [](int button)
    { return IsMouseButtonPressed(button); };
    input["is_mouse_button_released"] = [](int button)
    { return IsMouseButtonReleased(button); };

    // Mouse position and delta
    input["get_mouse_position"] = []() -> std::tuple<float, float>
    {
      auto pos = GetMousePosition();
      return std::make_tuple(pos.x, pos.y);
    };
    input["get_mouse_delta"] = []() -> std::tuple<float, float>
    {
      auto delta = GetMouseDelta();
      return std::make_tuple(delta.x, delta.y);
    };
    input["get_mouse_wheel"] = []()
    { return GetMouseWheelMove(); };

    // Letter keys
    input["KEY_A"] = KEY_A;
    input["KEY_B"] = KEY_B;
    input["KEY_C"] = KEY_C;
    input["KEY_D"] = KEY_D;
    input["KEY_E"] = KEY_E;
    input["KEY_F"] = KEY_F;
    input["KEY_G"] = KEY_G;
    input["KEY_H"] = KEY_H;
    input["KEY_I"] = KEY_I;
    input["KEY_J"] = KEY_J;
    input["KEY_K"] = KEY_K;
    input["KEY_L"] = KEY_L;
    input["KEY_M"] = KEY_M;
    input["KEY_N"] = KEY_N;
    input["KEY_O"] = KEY_O;
    input["KEY_P"] = KEY_P;
    input["KEY_Q"] = KEY_Q;
    input["KEY_R"] = KEY_R;
    input["KEY_S"] = KEY_S;
    input["KEY_T"] = KEY_T;
    input["KEY_U"] = KEY_U;
    input["KEY_V"] = KEY_V;
    input["KEY_W"] = KEY_W;
    input["KEY_X"] = KEY_X;
    input["KEY_Y"] = KEY_Y;
    input["KEY_Z"] = KEY_Z;

    // Special keys
    input["KEY_SPACE"] = KEY_SPACE;
    input["KEY_ESCAPE"] = KEY_ESCAPE;
    input["KEY_ENTER"] = KEY_ENTER;
    input["KEY_TAB"] = KEY_TAB;
    input["KEY_BACKSPACE"] = KEY_BACKSPACE;
    input["KEY_DELETE"] = KEY_DELETE;
    input["KEY_INSERT"] = KEY_INSERT;

    // Modifier keys
    input["KEY_LEFT_SHIFT"] = KEY_LEFT_SHIFT;
    input["KEY_RIGHT_SHIFT"] = KEY_RIGHT_SHIFT;
    input["KEY_LEFT_CONTROL"] = KEY_LEFT_CONTROL;
    input["KEY_RIGHT_CONTROL"] = KEY_RIGHT_CONTROL;
    input["KEY_LEFT_ALT"] = KEY_LEFT_ALT;
    input["KEY_RIGHT_ALT"] = KEY_RIGHT_ALT;

    // Arrow keys
    input["KEY_UP"] = KEY_UP;
    input["KEY_DOWN"] = KEY_DOWN;
    input["KEY_LEFT"] = KEY_LEFT;
    input["KEY_RIGHT"] = KEY_RIGHT;

    // Number keys
    input["KEY_ZERO"] = KEY_ZERO;
    input["KEY_ONE"] = KEY_ONE;
    input["KEY_TWO"] = KEY_TWO;
    input["KEY_THREE"] = KEY_THREE;
    input["KEY_FOUR"] = KEY_FOUR;
    input["KEY_FIVE"] = KEY_FIVE;
    input["KEY_SIX"] = KEY_SIX;
    input["KEY_SEVEN"] = KEY_SEVEN;
    input["KEY_EIGHT"] = KEY_EIGHT;
    input["KEY_NINE"] = KEY_NINE;

    // Function keys
    input["KEY_F1"] = KEY_F1;
    input["KEY_F2"] = KEY_F2;
    input["KEY_F3"] = KEY_F3;
    input["KEY_F4"] = KEY_F4;
    input["KEY_F5"] = KEY_F5;
    input["KEY_F6"] = KEY_F6;
    input["KEY_F7"] = KEY_F7;
    input["KEY_F8"] = KEY_F8;
    input["KEY_F9"] = KEY_F9;
    input["KEY_F10"] = KEY_F10;
    input["KEY_F11"] = KEY_F11;
    input["KEY_F12"] = KEY_F12;

    // Mouse button constants
    input["MOUSE_BUTTON_LEFT"] = MOUSE_BUTTON_LEFT;
    input["MOUSE_BUTTON_RIGHT"] = MOUSE_BUTTON_RIGHT;
    input["MOUSE_BUTTON_MIDDLE"] = MOUSE_BUTTON_MIDDLE;
  }

} // namespace moiras
