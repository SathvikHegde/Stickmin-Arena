#pragma once
#include "Fighter.hpp"
#include <RagdollEngine/Physics/ActiveRagdollController.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>

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

    void reset() {
        m_actionCooldown = 0.0f;
        m_reactionTimer = 0.0f;
        m_defenseTimer = 0.0f;
        m_defending = false;
        m_ducking = false;
        m_juggleStep = 0;
        m_juggleTimer = 0.0f;
        m_footsieTimer = 0.0f;
        m_targetMoveX = 0.0f;
        m_targetMoveY = 0.0f;
    }

    void update(float dt, Fighter& cpu, Fighter& opponent, RagdollEngine::StageRenderer* stageRenderer = nullptr) {
        auto* cpuCtrl = cpu.getController();
        auto* oppCtrl = opponent.getController();
        if (!cpuCtrl || !oppCtrl) return;

        // Don't issue inputs if CPU is incapacitated
        auto cpuState = cpuCtrl->getActionState();
        if (cpuState == RagdollEngine::FighterActionState::HitStun ||
            cpuState == RagdollEngine::FighterActionState::LaunchedJuggle ||
            cpuState == RagdollEngine::FighterActionState::KnockedDown ||
            cpuState == RagdollEngine::FighterActionState::GettingUp ||
            cpuState == RagdollEngine::FighterActionState::Limp) {
            m_targetMoveX = 0.0f;
            m_targetMoveY = 0.0f;
            cpuCtrl->setMoveInput(0.0f, 0.0f);
            return;
        }

        // Timers decay
        if (m_actionCooldown > 0.0f) m_actionCooldown -= dt;
        if (m_reactionTimer > 0.0f) m_reactionTimer -= dt;
        if (m_defenseTimer > 0.0f) {
            m_defenseTimer -= dt;
            if (m_defenseTimer <= 0.0f) {
                m_defending = false;
                m_ducking = false;
            }
        }
        if (m_footsieTimer > 0.0f) m_footsieTimer -= dt;

        sf::Vector2f cpuPos = cpu.getSkeleton()->getPositionPixels();
        sf::Vector2f oppPos = opponent.getSkeleton()->getPositionPixels();
        float dx = oppPos.x - cpuPos.x;
        float dist = std::abs(dx);
        int facing = cpuCtrl->getFacingDirection();
        int toOppDir = (dx >= 0.0f) ? 1 : -1;

        auto oppState = oppCtrl->getActionState();
        bool oppIsAttacking = oppCtrl->isAttacking();

        // =====================================================================
        // 1. AIR JUGGLE COMBO ENGINE (When opponent is launched in the air!)
        // =====================================================================
        if (oppState == RagdollEngine::FighterActionState::LaunchedJuggle) {
            m_juggleTimer += dt;

            // Move beneath opponent
            if (dist > 65.0f) {
                cpuCtrl->setMoveInput(static_cast<float>(toOppDir), 0.0f);
            } else {
                cpuCtrl->setMoveInput(0.0f, 0.0f);
            }

            if (dist < 110.0f && !cpuCtrl->isAttacking() && m_actionCooldown <= 0.0f) {
                if (m_difficulty == CpuDifficulty::Hard) {
                    // God-tier 3-hit juggle sequence
                    if (m_juggleStep == 0) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::FlashJab);
                        m_actionCooldown = 0.16f;
                        m_juggleStep = 1;
                    } else if (m_juggleStep == 1) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::StraightCross);
                        m_actionCooldown = 0.18f;
                        m_juggleStep = 2;
                    } else {
                        // Juggle finisher: Axe Roundhouse or Dropkick!
                        if (dist < 80.0f) {
                            cpuCtrl->triggerMove(RagdollEngine::MoveId::AxeRoundhouse);
                        } else {
                            cpuCtrl->triggerMove(RagdollEngine::MoveId::FlyingDropkick);
                        }
                        m_actionCooldown = 0.35f;
                        m_juggleStep = 0;
                    }
                } else if (m_difficulty == CpuDifficulty::Medium) {
                    if (m_juggleStep == 0) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::FlashJab);
                        m_actionCooldown = 0.20f;
                        m_juggleStep = 1;
                    } else {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::MidKick);
                        m_actionCooldown = 0.35f;
                        m_juggleStep = 0;
                    }
                } else {
                    // Easy: occasional single jab
                    if ((rand() % 100) < 40) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::FlashJab);
                    }
                    m_actionCooldown = 0.40f;
                }
            }
            return;
        } else {
            m_juggleStep = 0;
            m_juggleTimer = 0.0f;
        }

        // =====================================================================
        // 2. REACTIVE DEFENSE SYSTEM (Reading Opponent Attacks)
        // =====================================================================
        if (oppIsAttacking && dist < 140.0f) {
            const auto& oppMove = oppCtrl->getCurrentMove();

            if (!m_defending && m_reactionTimer <= 0.0f) {
                // Determine defense probability based on difficulty
                int blockRoll = rand() % 100;
                int blockChance = (m_difficulty == CpuDifficulty::Hard) ? 85
                                : (m_difficulty == CpuDifficulty::Medium) ? 60 : 25;

                if (blockRoll < blockChance) {
                    m_defending = true;
                    m_defenseTimer = oppMove.startup + oppMove.active + 0.05f;

                    // Counter-offensive reaction on Hard:
                    // If CPU is in Rage and has Rage Art, use Rage Art hyper armor reversal!
                    if (m_difficulty == CpuDifficulty::Hard && cpu.isInRage() && !cpu.hasUsedRageArt() && m_actionCooldown <= 0.0f) {
                        if ((rand() % 100) < 65) {
                            cpuCtrl->triggerMove(RagdollEngine::MoveId::RageArt);
                            m_actionCooldown = 0.60f;
                            return;
                        }
                    }

                    // On Hard/Medium: Absorb with Power Crush armor if high/mid
                    if (m_difficulty >= CpuDifficulty::Medium && (oppMove.height == RagdollEngine::AttackHeight::High || oppMove.height == RagdollEngine::AttackHeight::Mid)) {
                        if ((rand() % 100) < (m_difficulty == CpuDifficulty::Hard ? 35 : 20) && m_actionCooldown <= 0.0f) {
                            cpuCtrl->triggerMove(RagdollEngine::MoveId::PowerCrush);
                            m_actionCooldown = 0.45f;
                            return;
                        }
                    }

                    // If opponent throws: Duck under throw or jab to throw break!
                    if (oppMove.isThrow) {
                        if (m_difficulty == CpuDifficulty::Hard && (rand() % 100) < 50 && m_actionCooldown <= 0.0f) {
                            cpuCtrl->triggerMove(RagdollEngine::MoveId::FlashJab); // Breaks the throw!
                            m_actionCooldown = 0.22f;
                            return;
                        } else {
                            m_ducking = true; // Duck under the high grab!
                        }
                    } else if (oppMove.height == RagdollEngine::AttackHeight::High) {
                        // On Hard: Duck under High strikes to get a clean whiff punish!
                        if (m_difficulty == CpuDifficulty::Hard && (rand() % 100) < 60) {
                            m_ducking = true;
                        }
                    } else if (oppMove.height == RagdollEngine::AttackHeight::Low) {
                        // Crouch block Low attacks (Hell Sweep)
                        m_ducking = true;
                    }
                } else {
                    // Missed reaction
                    float delay = (m_difficulty == CpuDifficulty::Hard) ? 0.08f
                                : (m_difficulty == CpuDifficulty::Medium) ? 0.20f : 0.38f;
                    m_reactionTimer = delay;
                }
            }

            if (m_defending) {
                if (m_ducking) {
                    // Low Guard: Down + Back
                    cpuCtrl->setMoveInput(-static_cast<float>(facing), 1.0f);
                } else {
                    // High Guard: Back
                    cpuCtrl->setMoveInput(-static_cast<float>(facing), 0.0f);
                }
                return;
            }
        } else {
            m_defending = false;
            m_ducking = false;
        }

        // =====================================================================
        // 3. WHIFF PUNISH ENGINE (Opponent missed or is in Recovery/HitStun)
        // =====================================================================
        bool oppInRecovery = (oppIsAttacking && !oppCtrl->isAttackActive() && !oppCtrl->isAttackStartup());
        bool oppInStun = (oppState == RagdollEngine::FighterActionState::HitStun);

        if ((oppInRecovery || oppInStun) && dist < 125.0f && !cpuCtrl->isAttacking() && m_actionCooldown <= 0.0f) {
            int punishRoll = rand() % 100;
            if (m_difficulty == CpuDifficulty::Hard && punishRoll < 85) {
                // High-damage launcher whiff punish!
                if (dist < 100.0f) {
                    cpuCtrl->triggerMove(RagdollEngine::MoveId::ElectricWindGodFist);
                } else {
                    cpuCtrl->triggerMove(RagdollEngine::MoveId::Hopkick);
                }
                m_actionCooldown = 0.38f;
                return;
            } else if (m_difficulty == CpuDifficulty::Medium && punishRoll < 60) {
                if (dist < 85.0f) {
                    cpuCtrl->triggerMove(RagdollEngine::MoveId::OneTwoString);
                } else {
                    cpuCtrl->triggerMove(RagdollEngine::MoveId::StraightCross);
                }
                m_actionCooldown = 0.30f;
                return;
            }
        }

        // =====================================================================
        // 4. OKIZEME / WAKE-UP PRESSURE (Opponent Knocked Down / Getting Up)
        // =====================================================================
        if (oppState == RagdollEngine::FighterActionState::KnockedDown ||
            oppState == RagdollEngine::FighterActionState::GettingUp) {
            // Close in for wake-up mixup
            if (dist > 85.0f) {
                cpuCtrl->setMoveInput(static_cast<float>(toOppDir) * 0.8f, 0.0f);
            } else {
                cpuCtrl->setMoveInput(0.0f, 0.0f);
            }

            // Time a meaty mixup right as opponent gets up
            if (oppState == RagdollEngine::FighterActionState::GettingUp && dist < 95.0f && m_actionCooldown <= 0.0f) {
                if (m_difficulty >= CpuDifficulty::Medium) {
                    // 50/50 Oki mixup: Hell Sweep (Low) vs Hopkick (Mid launcher)
                    if ((rand() % 100) < 50) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::HellSweep);
                    } else {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::Hopkick);
                    }
                    m_actionCooldown = 0.40f;
                    return;
                }
            }
            return;
        }

        // =====================================================================
        // 5. WALL POSITIONING AWARENESS
        // =====================================================================
        bool cpuNearLeftWall = (cpuPos.x < 220.0f);
        bool cpuNearRightWall = (cpuPos.x > 1380.0f);
        bool oppNearWall = (oppPos.x < 220.0f || oppPos.x > 1380.0f);

        // If CPU is pinned against the wall, try to dash forward or jump out
        if ((cpuNearLeftWall && facing < 0) || (cpuNearRightWall && facing > 0)) {
            if (m_actionCooldown <= 0.0f && (rand() % 100) < 60) {
                cpuCtrl->triggerDash(facing);
                if (stageRenderer) {
                    stageRenderer->spawnGroundDust(cpuPos + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(-facing * 110.0f, -20.0f), 6);
                }
                m_actionCooldown = 0.28f;
                return;
            }
        }

        // If Opponent is near wall, unleash heavy wall splat moves!
        if (oppNearWall && dist < 120.0f && !cpuCtrl->isAttacking() && m_actionCooldown <= 0.0f) {
            int wallAttack = rand() % 100;
            if (wallAttack < 40) {
                cpuCtrl->triggerMove(RagdollEngine::MoveId::PowerCrush); // Wall Splat monster!
            } else if (wallAttack < 75) {
                cpuCtrl->triggerMove(RagdollEngine::MoveId::FlyingDropkick); // Wall Splat dropkick!
            } else {
                cpuCtrl->triggerMove(RagdollEngine::MoveId::AxeRoundhouse);
            }
            m_actionCooldown = 0.45f;
            return;
        }

        // =====================================================================
        // 6. RAGE ART OPPORTUNITY (Clutch Super)
        // =====================================================================
        if (cpu.isInRage() && !cpu.hasUsedRageArt() && dist < 115.0f && !cpuCtrl->isAttacking() && m_actionCooldown <= 0.0f) {
            int rageChance = (m_difficulty == CpuDifficulty::Hard) ? 45
                           : (m_difficulty == CpuDifficulty::Medium) ? 25 : 10;
            if ((rand() % 100) < rageChance) {
                cpuCtrl->triggerMove(RagdollEngine::MoveId::RageArt);
                m_actionCooldown = 0.65f;
                return;
            }
        }

        // =====================================================================
        // 7. NEUTRAL FOOTSIES & ATTACK LOGIC (Based on Distance)
        // =====================================================================
        if (m_actionCooldown <= 0.0f && !cpuCtrl->isAttacking()) {

            // -----------------------------------------------------------------
            // ZONE A: IN-FIGHTING / CLOSE RANGE (dist < 75 px)
            // -----------------------------------------------------------------
            if (dist < 75.0f) {
                int roll = rand() % 100;

                if (m_difficulty == CpuDifficulty::Hard) {
                    if (roll < 22) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::Throw); // Command Throw unblockable mixup!
                    } else if (roll < 45) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::FlashJab);
                    } else if (roll < 65) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::OneTwoString);
                    } else if (roll < 82) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::ElectricWindGodFist);
                    } else {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::HellSweep); // Low trip
                    }
                    m_actionCooldown = 0.22f + (rand() % 12) * 0.01f;
                } else if (m_difficulty == CpuDifficulty::Medium) {
                    if (roll < 15) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::Throw);
                    } else if (roll < 45) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::FlashJab);
                    } else if (roll < 70) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::StraightCross);
                    } else if (roll < 85) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::MidKick);
                    } else {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::HellSweep);
                    }
                    m_actionCooldown = 0.30f + (rand() % 15) * 0.01f;
                } else {
                    // Easy
                    if (roll < 50) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::FlashJab);
                    } else if (roll < 80) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::MidKick);
                    } else {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::StraightCross);
                    }
                    m_actionCooldown = 0.48f + (rand() % 20) * 0.01f;
                }
                return;
            }

            // -----------------------------------------------------------------
            // ZONE B: MID RANGE / POKE & LAUNCHER (75 px <= dist < 155 px)
            // -----------------------------------------------------------------
            else if (dist < 155.0f) {
                int roll = rand() % 100;

                if (m_difficulty == CpuDifficulty::Hard) {
                    if (roll < 30) {
                        // Mishima EWGF Launcher!
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::ElectricWindGodFist);
                    } else if (roll < 52) {
                        // Hopkick Mid launcher
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::Hopkick);
                    } else if (roll < 72) {
                        // Hell Sweep Low Trip
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::HellSweep);
                    } else if (roll < 86) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::MidKick);
                    } else {
                        // Forward dash into close range
                        cpuCtrl->triggerDash(facing);
                        if (stageRenderer) stageRenderer->spawnGroundDust(cpuPos + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(-facing * 110.0f, -20.0f), 6);
                    }
                    m_actionCooldown = 0.25f + (rand() % 14) * 0.01f;
                } else if (m_difficulty == CpuDifficulty::Medium) {
                    if (roll < 25) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::Hopkick);
                    } else if (roll < 45) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::HellSweep);
                    } else if (roll < 70) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::MidKick);
                    } else if (roll < 85) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::StraightCross);
                    } else {
                        cpuCtrl->setMoveInput(static_cast<float>(toOppDir), 0.0f);
                    }
                    m_actionCooldown = 0.34f + (rand() % 15) * 0.01f;
                } else {
                    // Easy
                    if (roll < 40) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::MidKick);
                    } else if (roll < 70) {
                        cpuCtrl->triggerMove(RagdollEngine::MoveId::StraightCross);
                    } else {
                        cpuCtrl->setMoveInput(static_cast<float>(toOppDir) * 0.6f, 0.0f);
                    }
                    m_actionCooldown = 0.50f + (rand() % 25) * 0.01f;
                }
                return;
            }

            // -----------------------------------------------------------------
            // ZONE C: FAR RANGE (dist >= 155 px)
            // -----------------------------------------------------------------
            else {
                int roll = rand() % 100;

                // Long range surprise dropkick!
                if (dist > 200.0f && roll < (m_difficulty == CpuDifficulty::Hard ? 35 : (m_difficulty == CpuDifficulty::Medium ? 20 : 10))) {
                    cpuCtrl->triggerMove(RagdollEngine::MoveId::FlyingDropkick);
                    m_actionCooldown = 0.55f;
                    return;
                }

                // Dashing forward to close the distance
                if (roll < (m_difficulty == CpuDifficulty::Hard ? 50 : 30)) {
                    cpuCtrl->triggerDash(toOppDir);
                    if (stageRenderer) stageRenderer->spawnGroundDust(cpuPos + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(-toOppDir * 110.0f, -20.0f), 6);
                    m_actionCooldown = 0.28f;
                    return;
                }

                // Walk forward toward opponent
                cpuCtrl->setMoveInput(static_cast<float>(toOppDir), 0.0f);
                return;
            }
        }

        // ---------------------------------------------------------------------
        // FOOTSIE MOVEMENT (Micro-spacing while waiting for action cooldown)
        // ---------------------------------------------------------------------
        if (m_footsieTimer <= 0.0f) {
            m_footsieTimer = 0.15f + (rand() % 20) * 0.01f;

            if (dist > 130.0f) {
                // Advance
                m_targetMoveX = static_cast<float>(toOppDir);
                m_targetMoveY = 0.0f;
            } else if (dist < 55.0f) {
                // Micro back-step to space out
                m_targetMoveX = -static_cast<float>(toOppDir) * 0.6f;
                m_targetMoveY = 0.0f;
            } else {
                // Small jitter / idle neutral guard
                int step = rand() % 3;
                if (step == 0) m_targetMoveX = static_cast<float>(toOppDir) * 0.4f;
                else if (step == 1) m_targetMoveX = -static_cast<float>(toOppDir) * 0.4f;
                else m_targetMoveX = 0.0f;
                m_targetMoveY = 0.0f;
            }
        }

        cpuCtrl->setMoveInput(m_targetMoveX, m_targetMoveY);
    }

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
