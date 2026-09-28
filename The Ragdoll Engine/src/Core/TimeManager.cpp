#include "RagdollEngine/Core/TimeManager.hpp"
#include <algorithm>

namespace RagdollEngine {

TimeManager::TimeManager() {
    m_clock.restart();
}

void TimeManager::update() {
    m_realDt = m_clock.restart().asSeconds();
    // Cap realDt to avoid spiral of death when window is dragged
    if (m_realDt > 0.1f) {
        m_realDt = 0.1f;
    }

    // Process Hitstop (Impact freeze)
    if (m_hitstopTimer > 0.0f) {
        m_hitstopTimer -= m_realDt;
        m_gameDt = 0.0f; // Freeze game simulation
        return;
    }

    // Process Slow-Motion Dilation
    if (m_slowMoTimer > 0.0f) {
        m_slowMoTimer -= m_realDt;
        // Hold at target scale, then smoothly ease back to base scale
        if (m_slowMoTimer <= 0.0f) {
            m_targetTimeScale = m_baseTimeScale;
        }
    }

    // Smoothly ease current scale towards target scale
    float lerpSpeed = 10.0f;
    m_currentTimeScale += (m_targetTimeScale - m_currentTimeScale) * std::min(1.0f, lerpSpeed * m_realDt);

    m_gameDt = m_realDt * m_currentTimeScale;
    m_accumulator += m_gameDt;
}

void TimeManager::triggerHitstop(float durationSeconds) {
    m_hitstopTimer = std::max(m_hitstopTimer, durationSeconds);
}

void TimeManager::triggerSlowMo(float targetScale, float durationSeconds) {
    m_targetTimeScale = targetScale;
    m_currentTimeScale = targetScale; // Instant snap into dramatic slow-mo
    m_slowMoTimer = durationSeconds;
    m_slowMoDuration = durationSeconds;
}

bool TimeManager::consumeFixedStep() {
    if (m_accumulator >= m_fixedStep) {
        m_accumulator -= m_fixedStep;
        return true;
    }
    return false;
}

} // namespace RagdollEngine
