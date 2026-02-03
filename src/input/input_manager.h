#pragma once

#include "input_types.h"
#include <raylib.h>

namespace moiras {

class InputManager {
public:
    // Singleton access
    static InputManager& getInstance();
    
    // Must be called once per frame BEFORE game logic
    void update();
    
    // Context management
    void setContext(InputContext context);
    InputContext getContext() const;
    
    // Action queries (returns false if ImGui is capturing and action is blockable)
    bool isActionActive(InputAction action) const;      // Held down
    bool isActionJustPressed(InputAction action) const; // Just pressed this frame
    bool isActionJustReleased(InputAction action) const; // Just released
    
    // Mouse/input state (respects ImGui capture)
    Vector2 getMouseDelta() const;
    float getMouseWheelMove() const;
    Vector2 getMousePosition() const;
    
    // Direct ImGui capture state queries
    bool isMouseCapturedByUI() const;
    bool isKeyboardCapturedByUI() const;
    
private:
    InputManager();
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    
    InputContext m_currentContext;
    bool m_mouseCapture;
    bool m_keyboardCapture;
    
    // Check if action is allowed in current context
    bool isActionAvailable(InputAction action) const;
    
    // Check if action should be blocked by ImGui
    bool isActionBlockedByUI(InputAction action) const;
    
    // Helper to determine if action uses mouse
    bool isMouseAction(InputAction action) const;
    
    // Helper to determine if action uses keyboard
    bool isKeyboardAction(InputAction action) const;
    
    // Get the raw input state for an action (ignoring context/capture)
    bool getRawActionState(InputAction action) const;
    bool getRawActionJustPressed(InputAction action) const;
    bool getRawActionJustReleased(InputAction action) const;
};

} // namespace moiras
