#include "time_manager.h"
#include <raylib.h>

namespace moiras {

// Singleton implementation
TimeManager& TimeManager::getInstance() {
    static TimeManager instance;
    return instance;
}

void TimeManager::update() {
    // Cache real frame time
    m_realDeltaTime = GetFrameTime();
    
    // Calculate game delta time (0 if paused, scaled otherwise)
    m_gameDeltaTime = m_isPaused ? 0.0f : (m_realDeltaTime * m_timeScale);
}

void TimeManager::togglePause() {
    m_isPaused = !m_isPaused;
    TraceLog(LOG_INFO, "TimeManager: %s", m_isPaused ? "PAUSED" : "RESUMED");
}

void TimeManager::setPaused(bool paused) {
    m_isPaused = paused;
    TraceLog(LOG_INFO, "TimeManager: %s", m_isPaused ? "PAUSED" : "RESUMED");
}

void TimeManager::setTimeScale(float scale) {
    // Clamp to reasonable range
    if (scale < 0.1f) scale = 0.1f;
    if (scale > 10.0f) scale = 10.0f;
    
    m_timeScale = scale;
    
    // Setting speed automatically unpauses
    if (m_isPaused) {
        m_isPaused = false;
        TraceLog(LOG_INFO, "TimeManager: Resumed and set speed to %.1fx", m_timeScale);
    } else {
        TraceLog(LOG_INFO, "TimeManager: Speed set to %.1fx", m_timeScale);
    }
}

} // namespace moiras
