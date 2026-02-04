#pragma once

namespace moiras {

// Input contexts for organizing actions
enum class InputContext {
    GAME,      // Normal gameplay
    UI,        // When script editor or menus are open
    BUILDING   // Building mode
};

// All input actions in the game
enum class InputAction {
    // Camera actions (available in GAME and BUILDING contexts)
    CAMERA_PAN_FORWARD,
    CAMERA_PAN_BACK,
    CAMERA_PAN_LEFT,
    CAMERA_PAN_RIGHT,
    CAMERA_ROTATE,
    CAMERA_ZOOM,
    CAMERA_TOGGLE_CURSOR,
    
    // Character actions (GAME context only)
    CHARACTER_MOVE,
    
    // Building actions (BUILDING context only)
    BUILDING_ROTATE_CCW,
    BUILDING_ROTATE_CW,
    BUILDING_SCALE_MODIFIER,  // Shift key for scaling
    BUILDING_PLACE,
    BUILDING_CANCEL,
    
    // UI actions (always available)
    UI_TOGGLE_SCRIPT_EDITOR,
    UI_TOGGLE_PAUSE,      // Space - pause/unpause game
    UI_SPEED_NORMAL,      // 1 - set speed to 1x
    UI_SPEED_MEDIUM,      // 2 - set speed to 2.5x
    UI_SPEED_FAST,        // 3 - set speed to 5x
    UI_CONFIRM,
    UI_CANCEL
};

} // namespace moiras
