#pragma once

#include <raylib.h>

namespace moiras {

/**
 * TimeManager - Singleton for managing game time scale and pause state
 * 
 * Provides two delta time values:
 * - getGameDeltaTime(): Scaled by speed multiplier, 0 when paused (for gameplay)
 * - getRealDeltaTime(): Unscaled, never 0 (for camera/UI)
 */
class TimeManager {
public:
    // Singleton access
    static TimeManager& getInstance();
    
    // Must be called once per frame BEFORE game logic
    void update();
    
    // Get delta time values
    float getGameDeltaTime() const { return m_gameDeltaTime; }
    float getRealDeltaTime() const { return m_realDeltaTime; }
    
    // Pause control
    void togglePause();
    void setPaused(bool paused);
    bool isPaused() const { return m_isPaused; }
    
    // Speed control
    void setTimeScale(float scale);
    float getTimeScale() const { return m_timeScale; }
    
private:
    TimeManager() = default;
    ~TimeManager() = default;
    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;
    
    bool m_isPaused = false;
    float m_timeScale = 1.0f;
    float m_realDeltaTime = 0.0f;
    float m_gameDeltaTime = 0.0f;
};

} // namespace moiras
