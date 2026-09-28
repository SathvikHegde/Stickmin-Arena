#pragma once
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

namespace RagdollEngine {

class TimeManager {
public:
    TimeManager();

    // Call at the start of each frame
    void update();

    // Get time deltas
    float getRealDeltaTime() const { return m_realDt; }
    float getGameDeltaTime() const { return m_gameDt; }
    float getFixedPhysicsStep() const { return m_fixedStep; }

    // Hitstop / Freeze-frame (combat impact juice)
    void triggerHitstop(float durationSeconds);
    bool isHitstopActive() const { return m_hitstopTimer > 0.0f; }

    // Slow-Mo / Time Dilation
    void triggerSlowMo(float targetScale, float durationSeconds);
    float getTimeScale() const { return m_currentTimeScale; }
    void setBaseTimeScale(float scale) { m_baseTimeScale = scale; }

    // Fixed timestep accumulator for Box2D
    bool consumeFixedStep();

private:
    sf::Clock m_clock;
    float m_realDt{ 0.016f };
    float m_gameDt{ 0.016f };
    float m_fixedStep{ 1.0f / 60.0f };
    float m_accumulator{ 0.0f };

    // Hitstop
    float m_hitstopTimer{ 0.0f };

    // Slow-mo
    float m_baseTimeScale{ 1.0f };
    float m_currentTimeScale{ 1.0f };
    float m_targetTimeScale{ 1.0f };
    float m_slowMoTimer{ 0.0f };
    float m_slowMoDuration{ 0.0f };
};

} // namespace RagdollEngine
