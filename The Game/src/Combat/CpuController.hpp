#pragma once
#include "Fighter.hpp"
#include <RagdollEngine/Physics/ActiveRagdollController.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>

namespace StickminGame {

enum class CpuDifficulty {
    Easy,       // Casual: Slower reactions (~0.40s), 25% block rate, basic pokes
    Medium,     // Fighter: Balanced reactions (~0.22s), 60% block rate, 1-2 strings & sweeps
    Hard        // Mishima God: Fast reactions (~0.10s), 85% block, EWGF launchers, air juggles, 50/50 oki
};

inline const char* getDifficultyName(CpuDifficulty diff) {
    switch (diff) {
        case CpuDifficulty::Easy: return "EASY";
        case CpuDifficulty::Medium: return "NORMAL";
        case CpuDifficulty::Hard: return "HARD (GOD)";
        default: return "NORMAL";
    }
}

class CpuController {
public:
    CpuController(CpuDifficulty diff = CpuDifficulty::Medium)
        : m_difficulty(diff) {}

    CpuDifficulty getDifficulty() const { return m_difficulty; }
    void setDifficulty(CpuDifficulty diff) { m_difficulty = diff; }

    void cycleDifficulty() {
        if (m_difficulty == CpuDifficulty::Easy) m_difficulty = CpuDifficulty::Medium;
        else if (m_difficulty == CpuDifficulty::Medium) m_difficulty = CpuDifficulty::Hard;
        else m_difficulty = CpuDifficulty::Easy;
    }

    void reset();
    void update(float dt, Fighter& cpu, Fighter& opponent, RagdollEngine::StageRenderer* stageRenderer = nullptr);

private:
    CpuDifficulty m_difficulty{ CpuDifficulty::Medium };

    float m_actionCooldown{ 0.0f };
    float m_reactionTimer{ 0.0f };
    float m_defenseTimer{ 0.0f };
    bool m_defending{ false };
    bool m_ducking{ false };

    int m_juggleStep{ 0 };
    float m_juggleTimer{ 0.0f };

    float m_footsieTimer{ 0.0f };
    float m_targetMoveX{ 0.0f };
    float m_targetMoveY{ 0.0f };
};

} // namespace StickminGame
