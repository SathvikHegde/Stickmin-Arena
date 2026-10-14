#pragma once
#include "Fighter.hpp"
#include "NetworkProtocol.hpp"
#include <RagdollEngine/Core/TimeManager.hpp>
#include <RagdollEngine/Core/CameraDirector.hpp>
#include <RagdollEngine/Render/JuiceFX.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>
#include <vector>
#include <string>

namespace StickminGame {

enum class MatchState {
    RoundIntro,
    Fighting,
    RoundKO,
    MatchOver
};

class CombatManager {
public:
    CombatManager(Fighter* p1, Fighter* p2);

    void startRound(int roundNumber);
    void update(float dt, RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX, RagdollEngine::StageRenderer* stageRenderer = nullptr);

    MatchState getState() const { return m_state; }
    void setState(MatchState s) { m_state = s; }
    float getStateTimer() const { return m_stateTimer; }
    float getRoundTimer() const { return m_roundTimer; }
    void setRoundTimer(float t) { m_roundTimer = t; }
    int getRoundNumber() const { return m_roundNumber; }
    void setRoundNumber(int n) { m_roundNumber = n; }
    int getRoundWinner() const { return m_roundWinner; }
    void setRoundWinner(int w) { m_roundWinner = w; }

    std::vector<JuiceFXEvent> consumePendingJuiceEvents();
    void queueJuiceEvent(JuiceEventType type, sf::Vector2f pos, sf::Vector2f normal, sf::Color col, float intensity = 1.0f, const std::string& text = "");

private:
    Fighter* m_p1{ nullptr };
    Fighter* m_p2{ nullptr };

    MatchState m_state{ MatchState::RoundIntro };
    int m_roundNumber{ 1 };
    float m_roundTimer{ 60.0f };
    float m_stateTimer{ 0.0f };
    int m_roundWinner{ 0 };

    float m_clashCooldown{ 0.0f };
    std::vector<JuiceFXEvent> m_pendingJuiceEvents;

    void checkSlowMoClash(RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX);
    void processStrikes(Fighter& attacker, Fighter& defender, RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX, RagdollEngine::StageRenderer* stageRenderer = nullptr);
};

} // namespace StickminGame
