#include "input_manager.h"
#include <imgui.h>

namespace moiras {

// Singleton implementation
InputManager& InputManager::getInstance() {
    static InputManager instance;
    return instance;
}

InputManager::InputManager() 
    : m_currentContext(InputContext::GAME)
    , m_mouseCapture(false)
    , m_keyboardCapture(false)
{
}

void InputManager::update() {
    // Cache ImGui capture state for this frame
    m_mouseCapture = ImGui::GetIO().WantCaptureMouse;
    m_keyboardCapture = ImGui::GetIO().WantCaptureKeyboard;
}

void InputManager::setContext(InputContext context) {
    m_currentContext = context;
}

InputContext InputManager::getContext() const {
    return m_currentContext;
}

bool InputManager::isActionActive(InputAction action) const {
    // Check if available in current context
    if (!isActionAvailable(action)) return false;
    
    // Check if blocked by ImGui
    if (isActionBlockedByUI(action)) return false;
    
    // Get raw state
    return getRawActionState(action);
}

bool InputManager::isActionJustPressed(InputAction action) const {
    // Check if available in current context
    if (!isActionAvailable(action)) return false;
    
    // Check if blocked by ImGui
    if (isActionBlockedByUI(action)) return false;
    
    // Get raw state
    return getRawActionJustPressed(action);
}

bool InputManager::isActionJustReleased(InputAction action) const {
    // Check if available in current context
    if (!isActionAvailable(action)) return false;
    
    // Check if blocked by ImGui
    if (isActionBlockedByUI(action)) return false;
    
    // Get raw state
    return getRawActionJustReleased(action);
}

Vector2 InputManager::getMouseDelta() const {
    // If mouse is captured by UI, return zero delta
    if (m_mouseCapture) {
        return Vector2{0.0f, 0.0f};
    }
    return GetMouseDelta();
}

float InputManager::getMouseWheelMove() const {
    // If mouse is captured by UI, return zero
    if (m_mouseCapture) {
        return 0.0f;
    }
    return GetMouseWheelMove();
}

Vector2 InputManager::getMousePosition() const {
    return GetMousePosition();
}

bool InputManager::isMouseCapturedByUI() const {
    return m_mouseCapture;
}

bool InputManager::isKeyboardCapturedByUI() const {
    return m_keyboardCapture;
}

bool InputManager::isActionAvailable(InputAction action) const {
    switch (action) {
        // Camera actions available in GAME and BUILDING
        case InputAction::CAMERA_PAN_FORWARD:
        case InputAction::CAMERA_PAN_BACK:
        case InputAction::CAMERA_PAN_LEFT:
        case InputAction::CAMERA_PAN_RIGHT:
        case InputAction::CAMERA_ROTATE:
        case InputAction::CAMERA_ZOOM:
        case InputAction::CAMERA_TOGGLE_CURSOR:
            return m_currentContext == InputContext::GAME || 
                   m_currentContext == InputContext::BUILDING;
        
        // Character actions only in GAME
        case InputAction::CHARACTER_MOVE:
            return m_currentContext == InputContext::GAME;
        
        // Building actions only in BUILDING
        case InputAction::BUILDING_ROTATE_CCW:
        case InputAction::BUILDING_ROTATE_CW:
        case InputAction::BUILDING_SCALE_MODIFIER:
        case InputAction::BUILDING_PLACE:
        case InputAction::BUILDING_CANCEL:
            return m_currentContext == InputContext::BUILDING;
        
        // UI actions always available
        case InputAction::UI_TOGGLE_SCRIPT_EDITOR:
        case InputAction::UI_CONFIRM:
        case InputAction::UI_CANCEL:
            return true;
    }
    return false;
}

bool InputManager::isActionBlockedByUI(InputAction action) const {
    // UI actions are NEVER blocked
    if (action == InputAction::UI_TOGGLE_SCRIPT_EDITOR) return false;
    if (action == InputAction::UI_CONFIRM) return false;
    if (action == InputAction::UI_CANCEL) return false;
    
    // Mouse actions blocked when UI wants mouse
    if (isMouseAction(action) && m_mouseCapture) return true;
    
    // Keyboard actions blocked when UI wants keyboard
    if (isKeyboardAction(action) && m_keyboardCapture) return true;
    
    return false;
}

bool InputManager::isMouseAction(InputAction action) const {
    switch (action) {
        case InputAction::CAMERA_ROTATE:
        case InputAction::CAMERA_ZOOM:
        case InputAction::CHARACTER_MOVE:
        case InputAction::BUILDING_PLACE:
        case InputAction::BUILDING_CANCEL: // Right click can cancel
            return true;
        default:
            return false;
    }
}

bool InputManager::isKeyboardAction(InputAction action) const {
    switch (action) {
        case InputAction::CAMERA_PAN_FORWARD:
        case InputAction::CAMERA_PAN_BACK:
        case InputAction::CAMERA_PAN_LEFT:
        case InputAction::CAMERA_PAN_RIGHT:
        case InputAction::CAMERA_TOGGLE_CURSOR:
        case InputAction::BUILDING_ROTATE_CCW:
        case InputAction::BUILDING_ROTATE_CW:
        case InputAction::BUILDING_SCALE_MODIFIER:
        case InputAction::BUILDING_CANCEL: // ESC can also cancel
        case InputAction::UI_TOGGLE_SCRIPT_EDITOR:
        case InputAction::UI_CONFIRM:
        case InputAction::UI_CANCEL:
            return true;
        default:
            return false;
    }
}

bool InputManager::getRawActionState(InputAction action) const {
    switch (action) {
        // Camera panning
        case InputAction::CAMERA_PAN_FORWARD:
            return IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
        case InputAction::CAMERA_PAN_BACK:
            return IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
        case InputAction::CAMERA_PAN_LEFT:
            return IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
        case InputAction::CAMERA_PAN_RIGHT:
            return IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
        
        // Camera rotation
        case InputAction::CAMERA_ROTATE:
            return IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        
        // Camera zoom (handled via getMouseWheelMove)
        case InputAction::CAMERA_ZOOM:
            return false; // Not a held state
        
        // Camera toggle cursor
        case InputAction::CAMERA_TOGGLE_CURSOR:
            return IsKeyDown(KEY_P);
        
        // Character movement
        case InputAction::CHARACTER_MOVE:
            return IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
        
        // Building rotation
        case InputAction::BUILDING_ROTATE_CCW:
            return IsKeyDown(KEY_Q);
        case InputAction::BUILDING_ROTATE_CW:
            return IsKeyDown(KEY_E);
        
        // Building scale modifier
        case InputAction::BUILDING_SCALE_MODIFIER:
            return IsKeyDown(KEY_LEFT_SHIFT);
        
        // Building place
        case InputAction::BUILDING_PLACE:
            return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        
        // Building cancel
        case InputAction::BUILDING_CANCEL:
            return IsKeyDown(KEY_ESCAPE) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
        
        // UI actions
        case InputAction::UI_TOGGLE_SCRIPT_EDITOR:
            return IsKeyDown(KEY_F12);
        case InputAction::UI_CONFIRM:
            return IsKeyDown(KEY_ENTER);
        case InputAction::UI_CANCEL:
            return IsKeyDown(KEY_ESCAPE);
        
        default:
            return false;
    }
}

bool InputManager::getRawActionJustPressed(InputAction action) const {
    switch (action) {
        // Camera panning (not typically "just pressed", but supporting it)
        case InputAction::CAMERA_PAN_FORWARD:
            return IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
        case InputAction::CAMERA_PAN_BACK:
            return IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
        case InputAction::CAMERA_PAN_LEFT:
            return IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
        case InputAction::CAMERA_PAN_RIGHT:
            return IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
        
        // Camera rotation
        case InputAction::CAMERA_ROTATE:
            return IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
        
        // Camera zoom (not a pressed action)
        case InputAction::CAMERA_ZOOM:
            return false;
        
        // Camera toggle cursor
        case InputAction::CAMERA_TOGGLE_CURSOR:
            return IsKeyPressed(KEY_P);
        
        // Character movement
        case InputAction::CHARACTER_MOVE:
            return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        
        // Building rotation
        case InputAction::BUILDING_ROTATE_CCW:
            return IsKeyPressed(KEY_Q);
        case InputAction::BUILDING_ROTATE_CW:
            return IsKeyPressed(KEY_E);
        
        // Building scale modifier
        case InputAction::BUILDING_SCALE_MODIFIER:
            return IsKeyPressed(KEY_LEFT_SHIFT);
        
        // Building place
        case InputAction::BUILDING_PLACE:
            return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        
        // Building cancel
        case InputAction::BUILDING_CANCEL:
            return IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        
        // UI actions
        case InputAction::UI_TOGGLE_SCRIPT_EDITOR:
            return IsKeyPressed(KEY_F12);
        case InputAction::UI_CONFIRM:
            return IsKeyPressed(KEY_ENTER);
        case InputAction::UI_CANCEL:
            return IsKeyPressed(KEY_ESCAPE);
        
        default:
            return false;
    }
}

bool InputManager::getRawActionJustReleased(InputAction action) const {
    switch (action) {
        // Camera panning
        case InputAction::CAMERA_PAN_FORWARD:
            return IsKeyReleased(KEY_W) || IsKeyReleased(KEY_UP);
        case InputAction::CAMERA_PAN_BACK:
            return IsKeyReleased(KEY_S) || IsKeyReleased(KEY_DOWN);
        case InputAction::CAMERA_PAN_LEFT:
            return IsKeyReleased(KEY_A) || IsKeyReleased(KEY_LEFT);
        case InputAction::CAMERA_PAN_RIGHT:
            return IsKeyReleased(KEY_D) || IsKeyReleased(KEY_RIGHT);
        
        // Camera rotation
        case InputAction::CAMERA_ROTATE:
            return IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE);
        
        // Camera zoom
        case InputAction::CAMERA_ZOOM:
            return false;
        
        // Camera toggle cursor
        case InputAction::CAMERA_TOGGLE_CURSOR:
            return IsKeyReleased(KEY_P);
        
        // Character movement
        case InputAction::CHARACTER_MOVE:
            return IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
        
        // Building rotation
        case InputAction::BUILDING_ROTATE_CCW:
            return IsKeyReleased(KEY_Q);
        case InputAction::BUILDING_ROTATE_CW:
            return IsKeyReleased(KEY_E);
        
        // Building scale modifier
        case InputAction::BUILDING_SCALE_MODIFIER:
            return IsKeyReleased(KEY_LEFT_SHIFT);
        
        // Building place
        case InputAction::BUILDING_PLACE:
            return IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
        
        // Building cancel
        case InputAction::BUILDING_CANCEL:
            return IsKeyReleased(KEY_ESCAPE) || IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
        
        // UI actions
        case InputAction::UI_TOGGLE_SCRIPT_EDITOR:
            return IsKeyReleased(KEY_F12);
        case InputAction::UI_CONFIRM:
            return IsKeyReleased(KEY_ENTER);
        case InputAction::UI_CANCEL:
            return IsKeyReleased(KEY_ESCAPE);
        
        default:
            return false;
    }
}

} // namespace moiras
