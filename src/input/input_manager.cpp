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
    , m_mousePosition(0.0f, 0.0f)
    , m_mouseDelta(0.0f, 0.0f)
    , m_mouseWheelY(0.0f)
{
}

void InputManager::update() {
    // Cache ImGui capture state for this frame
    m_mouseCapture = ImGui::GetIO().WantCaptureMouse;
    m_keyboardCapture = ImGui::GetIO().WantCaptureKeyboard;

    // Swap current and previous state for edge detection
    m_keysPrevious = m_keysCurrent;
    m_mouseButtonsPrevious = m_mouseButtonsCurrent;

    // Reset per-frame state
    m_mouseDelta = Vec2(0.0f, 0.0f);
    m_mouseWheelY = 0.0f;
}

// SDL event handlers
void InputManager::handleSDLKeyDown(const SDL_KeyboardEvent& event) {
    m_keysCurrent[event.keysym.scancode] = true;
}

void InputManager::handleSDLKeyUp(const SDL_KeyboardEvent& event) {
    m_keysCurrent[event.keysym.scancode] = false;
}

void InputManager::handleSDLMouseMotion(const SDL_MouseMotionEvent& event) {
    m_mousePosition = Vec2((float)event.x, (float)event.y);
    m_mouseDelta.x += (float)event.xrel;
    m_mouseDelta.y += (float)event.yrel;
}

void InputManager::handleSDLMouseButtonDown(const SDL_MouseButtonEvent& event) {
    m_mouseButtonsCurrent[event.button] = true;
}

void InputManager::handleSDLMouseButtonUp(const SDL_MouseButtonEvent& event) {
    m_mouseButtonsCurrent[event.button] = false;
}

void InputManager::handleSDLMouseWheel(const SDL_MouseWheelEvent& event) {
    m_mouseWheelY += (float)event.y;
}

void InputManager::setContext(InputContext context) {
    m_currentContext = context;
}

InputContext InputManager::getContext() const {
    return m_currentContext;
}

bool InputManager::isActionActive(InputAction action) const {
    if (!isActionAvailable(action)) return false;
    if (isActionBlockedByUI(action)) return false;
    return getRawActionState(action);
}

bool InputManager::isActionJustPressed(InputAction action) const {
    if (!isActionAvailable(action)) return false;
    if (isActionBlockedByUI(action)) return false;
    return getRawActionJustPressed(action);
}

bool InputManager::isActionJustReleased(InputAction action) const {
    if (!isActionAvailable(action)) return false;
    if (isActionBlockedByUI(action)) return false;
    return getRawActionJustReleased(action);
}

Vec2 InputManager::getMouseDelta() const {
    if (m_mouseCapture) {
        return Vec2(0.0f, 0.0f);
    }
    return m_mouseDelta;
}

float InputManager::getMouseWheelMove() const {
    if (m_mouseCapture) {
        return 0.0f;
    }
    return m_mouseWheelY;
}

Vec2 InputManager::getMousePosition() const {
    return m_mousePosition;
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
        case InputAction::UI_TOGGLE_PAUSE:
        case InputAction::UI_SPEED_NORMAL:
        case InputAction::UI_SPEED_MEDIUM:
        case InputAction::UI_SPEED_FAST:
        case InputAction::UI_CONFIRM:
        case InputAction::UI_CANCEL:
            return true;
    }
    return false;
}

bool InputManager::isActionBlockedByUI(InputAction action) const {
    // UI actions are NEVER blocked
    if (action == InputAction::UI_TOGGLE_SCRIPT_EDITOR) return false;
    if (action == InputAction::UI_TOGGLE_PAUSE) return false;
    if (action == InputAction::UI_SPEED_NORMAL) return false;
    if (action == InputAction::UI_SPEED_MEDIUM) return false;
    if (action == InputAction::UI_SPEED_FAST) return false;
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
        case InputAction::UI_TOGGLE_PAUSE:
        case InputAction::UI_SPEED_NORMAL:
        case InputAction::UI_SPEED_MEDIUM:
        case InputAction::UI_SPEED_FAST:
        case InputAction::UI_CONFIRM:
        case InputAction::UI_CANCEL:
            return true;
        default:
            return false;
    }
}

// Helper to check if SDL key is down
bool InputManager::getRawActionState(InputAction action) const {
    auto isKeyDown = [this](SDL_Scancode code) {
        auto it = m_keysCurrent.find(code);
        return it != m_keysCurrent.end() && it->second;
    };

    auto isMouseDown = [this](Uint8 button) {
        auto it = m_mouseButtonsCurrent.find(button);
        return it != m_mouseButtonsCurrent.end() && it->second;
    };

    switch (action) {
        // Camera panning
        case InputAction::CAMERA_PAN_FORWARD:
            return isKeyDown(SDL_SCANCODE_W) || isKeyDown(SDL_SCANCODE_UP);
        case InputAction::CAMERA_PAN_BACK:
            return isKeyDown(SDL_SCANCODE_S) || isKeyDown(SDL_SCANCODE_DOWN);
        case InputAction::CAMERA_PAN_LEFT:
            return isKeyDown(SDL_SCANCODE_A) || isKeyDown(SDL_SCANCODE_LEFT);
        case InputAction::CAMERA_PAN_RIGHT:
            return isKeyDown(SDL_SCANCODE_D) || isKeyDown(SDL_SCANCODE_RIGHT);

        // Camera rotation
        case InputAction::CAMERA_ROTATE:
            return isMouseDown(SDL_BUTTON_MIDDLE);

        // Camera zoom (handled via getMouseWheelMove)
        case InputAction::CAMERA_ZOOM:
            return false; // Not a held state

        // Camera toggle cursor
        case InputAction::CAMERA_TOGGLE_CURSOR:
            return isKeyDown(SDL_SCANCODE_P);

        // Character movement
        case InputAction::CHARACTER_MOVE:
            return isMouseDown(SDL_BUTTON_RIGHT);

        // Building rotation
        case InputAction::BUILDING_ROTATE_CCW:
            return isKeyDown(SDL_SCANCODE_Q);
        case InputAction::BUILDING_ROTATE_CW:
            return isKeyDown(SDL_SCANCODE_E);

        // Building scale modifier
        case InputAction::BUILDING_SCALE_MODIFIER:
            return isKeyDown(SDL_SCANCODE_LSHIFT);

        // Building place
        case InputAction::BUILDING_PLACE:
            return isMouseDown(SDL_BUTTON_LEFT);

        // Building cancel
        case InputAction::BUILDING_CANCEL:
            return isKeyDown(SDL_SCANCODE_ESCAPE) || isMouseDown(SDL_BUTTON_RIGHT);

        // UI actions
        case InputAction::UI_TOGGLE_SCRIPT_EDITOR:
            return isKeyDown(SDL_SCANCODE_F12);
        case InputAction::UI_TOGGLE_PAUSE:
            return isKeyDown(SDL_SCANCODE_SPACE);
        case InputAction::UI_SPEED_NORMAL:
            return isKeyDown(SDL_SCANCODE_1);
        case InputAction::UI_SPEED_MEDIUM:
            return isKeyDown(SDL_SCANCODE_2);
        case InputAction::UI_SPEED_FAST:
            return isKeyDown(SDL_SCANCODE_3);
        case InputAction::UI_CONFIRM:
            return isKeyDown(SDL_SCANCODE_RETURN);
        case InputAction::UI_CANCEL:
            return isKeyDown(SDL_SCANCODE_ESCAPE);

        default:
            return false;
    }
}

bool InputManager::getRawActionJustPressed(InputAction action) const {
    auto wasKeyPressed = [this](SDL_Scancode code) {
        auto curr = m_keysCurrent.find(code);
        auto prev = m_keysPrevious.find(code);
        bool isDown = (curr != m_keysCurrent.end() && curr->second);
        bool wasDown = (prev != m_keysPrevious.end() && prev->second);
        return isDown && !wasDown;
    };

    auto wasMousePressed = [this](Uint8 button) {
        auto curr = m_mouseButtonsCurrent.find(button);
        auto prev = m_mouseButtonsPrevious.find(button);
        bool isDown = (curr != m_mouseButtonsCurrent.end() && curr->second);
        bool wasDown = (prev != m_mouseButtonsPrevious.end() && prev->second);
        return isDown && !wasDown;
    };

    switch (action) {
        // Camera panning
        case InputAction::CAMERA_PAN_FORWARD:
            return wasKeyPressed(SDL_SCANCODE_W) || wasKeyPressed(SDL_SCANCODE_UP);
        case InputAction::CAMERA_PAN_BACK:
            return wasKeyPressed(SDL_SCANCODE_S) || wasKeyPressed(SDL_SCANCODE_DOWN);
        case InputAction::CAMERA_PAN_LEFT:
            return wasKeyPressed(SDL_SCANCODE_A) || wasKeyPressed(SDL_SCANCODE_LEFT);
        case InputAction::CAMERA_PAN_RIGHT:
            return wasKeyPressed(SDL_SCANCODE_D) || wasKeyPressed(SDL_SCANCODE_RIGHT);

        // Camera rotation
        case InputAction::CAMERA_ROTATE:
            return wasMousePressed(SDL_BUTTON_MIDDLE);

        // Camera zoom
        case InputAction::CAMERA_ZOOM:
            return false;

        // Camera toggle cursor
        case InputAction::CAMERA_TOGGLE_CURSOR:
            return wasKeyPressed(SDL_SCANCODE_P);

        // Character movement
        case InputAction::CHARACTER_MOVE:
            return wasMousePressed(SDL_BUTTON_RIGHT);

        // Building rotation
        case InputAction::BUILDING_ROTATE_CCW:
            return wasKeyPressed(SDL_SCANCODE_Q);
        case InputAction::BUILDING_ROTATE_CW:
            return wasKeyPressed(SDL_SCANCODE_E);

        // Building scale modifier
        case InputAction::BUILDING_SCALE_MODIFIER:
            return wasKeyPressed(SDL_SCANCODE_LSHIFT);

        // Building place
        case InputAction::BUILDING_PLACE:
            return wasMousePressed(SDL_BUTTON_LEFT);

        // Building cancel
        case InputAction::BUILDING_CANCEL:
            return wasKeyPressed(SDL_SCANCODE_ESCAPE) || wasMousePressed(SDL_BUTTON_RIGHT);

        // UI actions
        case InputAction::UI_TOGGLE_SCRIPT_EDITOR:
            return wasKeyPressed(SDL_SCANCODE_F12);
        case InputAction::UI_TOGGLE_PAUSE:
            return wasKeyPressed(SDL_SCANCODE_SPACE);
        case InputAction::UI_SPEED_NORMAL:
            return wasKeyPressed(SDL_SCANCODE_1);
        case InputAction::UI_SPEED_MEDIUM:
            return wasKeyPressed(SDL_SCANCODE_2);
        case InputAction::UI_SPEED_FAST:
            return wasKeyPressed(SDL_SCANCODE_3);
        case InputAction::UI_CONFIRM:
            return wasKeyPressed(SDL_SCANCODE_RETURN);
        case InputAction::UI_CANCEL:
            return wasKeyPressed(SDL_SCANCODE_ESCAPE);

        default:
            return false;
    }
}

bool InputManager::getRawActionJustReleased(InputAction action) const {
    auto wasKeyReleased = [this](SDL_Scancode code) {
        auto curr = m_keysCurrent.find(code);
        auto prev = m_keysPrevious.find(code);
        bool isDown = (curr != m_keysCurrent.end() && curr->second);
        bool wasDown = (prev != m_keysPrevious.end() && prev->second);
        return !isDown && wasDown;
    };

    auto wasMouseReleased = [this](Uint8 button) {
        auto curr = m_mouseButtonsCurrent.find(button);
        auto prev = m_mouseButtonsPrevious.find(button);
        bool isDown = (curr != m_mouseButtonsCurrent.end() && curr->second);
        bool wasDown = (prev != m_mouseButtonsPrevious.end() && prev->second);
        return !isDown && wasDown;
    };

    switch (action) {
        // Camera panning
        case InputAction::CAMERA_PAN_FORWARD:
            return wasKeyReleased(SDL_SCANCODE_W) || wasKeyReleased(SDL_SCANCODE_UP);
        case InputAction::CAMERA_PAN_BACK:
            return wasKeyReleased(SDL_SCANCODE_S) || wasKeyReleased(SDL_SCANCODE_DOWN);
        case InputAction::CAMERA_PAN_LEFT:
            return wasKeyReleased(SDL_SCANCODE_A) || wasKeyReleased(SDL_SCANCODE_LEFT);
        case InputAction::CAMERA_PAN_RIGHT:
            return wasKeyReleased(SDL_SCANCODE_D) || wasKeyReleased(SDL_SCANCODE_RIGHT);

        // Camera rotation
        case InputAction::CAMERA_ROTATE:
            return wasMouseReleased(SDL_BUTTON_MIDDLE);

        // Camera zoom
        case InputAction::CAMERA_ZOOM:
            return false;

        // Camera toggle cursor
        case InputAction::CAMERA_TOGGLE_CURSOR:
            return wasKeyReleased(SDL_SCANCODE_P);

        // Character movement
        case InputAction::CHARACTER_MOVE:
            return wasMouseReleased(SDL_BUTTON_RIGHT);

        // Building rotation
        case InputAction::BUILDING_ROTATE_CCW:
            return wasKeyReleased(SDL_SCANCODE_Q);
        case InputAction::BUILDING_ROTATE_CW:
            return wasKeyReleased(SDL_SCANCODE_E);

        // Building scale modifier
        case InputAction::BUILDING_SCALE_MODIFIER:
            return wasKeyReleased(SDL_SCANCODE_LSHIFT);

        // Building place
        case InputAction::BUILDING_PLACE:
            return wasMouseReleased(SDL_BUTTON_LEFT);

        // Building cancel
        case InputAction::BUILDING_CANCEL:
            return wasKeyReleased(SDL_SCANCODE_ESCAPE) || wasMouseReleased(SDL_BUTTON_RIGHT);

        // UI actions
        case InputAction::UI_TOGGLE_SCRIPT_EDITOR:
            return wasKeyReleased(SDL_SCANCODE_F12);
        case InputAction::UI_TOGGLE_PAUSE:
            return wasKeyReleased(SDL_SCANCODE_SPACE);
        case InputAction::UI_SPEED_NORMAL:
            return wasKeyReleased(SDL_SCANCODE_1);
        case InputAction::UI_SPEED_MEDIUM:
            return wasKeyReleased(SDL_SCANCODE_2);
        case InputAction::UI_SPEED_FAST:
            return wasKeyReleased(SDL_SCANCODE_3);
        case InputAction::UI_CONFIRM:
            return wasKeyReleased(SDL_SCANCODE_RETURN);
        case InputAction::UI_CANCEL:
            return wasKeyReleased(SDL_SCANCODE_ESCAPE);

        default:
            return false;
    }
}

} // namespace moiras
