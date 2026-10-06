#pragma once
#include "Fighter.hpp"
#include <RagdollEngine/Core/TimeManager.hpp>
#include <RagdollEngine/Core/CameraDirector.hpp>
#include <RagdollEngine/Render/JuiceFX.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>
#include <cmath>
#include <algorithm>

namespace StickminGame {

enum class MatchState {
    RoundIntro,
    Fighting,
    RoundKO,
    MatchOver
};

class CombatManager {
public:
    CombatManager(Fighter* p1, Fighter* p2)
        : m_p1(p1), m_p2(p2) {}

    void startRound(int roundNumber) {
        m_roundNumber = roundNumber;
        m_roundTimer = 60.0f;
        m_state = MatchState::RoundIntro;
        m_stateTimer = 1.6f;
        m_clashCooldown = 0.0f;
    }

    void update(float dt, RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX, RagdollEngine::StageRenderer* stageRenderer = nullptr) {
        if (!m_p1 || !m_p2) return;

        sf::Vector2f p1Pos = m_p1->getSkeleton()->getPositionPixels();
        sf::Vector2f p2Pos = m_p2->getSkeleton()->getPositionPixels();

        // 1. Auto-Facing Direction (Fighting game staple)
        if (!m_p1->getController()->isAttacking() &&
            m_p1->getController()->getActionState() != RagdollEngine::FighterActionState::LaunchedJuggle &&
            m_p1->getController()->getActionState() != RagdollEngine::FighterActionState::KnockedDown &&
            m_p1->getController()->getActionState() != RagdollEngine::FighterActionState::GettingUp) {
            m_p1->getController()->setFacingDirection(p1Pos.x <= p2Pos.x ? 1 : -1);
        }

        if (!m_p2->getController()->isAttacking() &&
            m_p2->getController()->getActionState() != RagdollEngine::FighterActionState::LaunchedJuggle &&
            m_p2->getController()->getActionState() != RagdollEngine::FighterActionState::KnockedDown &&
            m_p2->getController()->getActionState() != RagdollEngine::FighterActionState::GettingUp) {
            m_p2->getController()->setFacingDirection(p2Pos.x <= p1Pos.x ? 1 : -1);
        }

        // 2. Match State Machine
        if (m_state == MatchState::RoundIntro) {
            m_stateTimer -= dt;
            if (m_stateTimer <= 0.0f) {
                m_state = MatchState::Fighting;
            }
            return;
        }

        if (m_state == MatchState::Fighting) {
            m_roundTimer = std::max(0.0f, m_roundTimer - dt);

            // Check KO or Time Out
            if (m_p1->isDead() || m_p2->isDead() || m_roundTimer <= 0.0f) {
                m_state = MatchState::RoundKO;
                m_stateTimer = 2.4f;

                if (m_p1->isDead() && !m_p2->isDead()) {
                    m_p2->addRoundWin();
                    m_roundWinner = 2;
                } else if (m_p2->isDead() && !m_p1->isDead()) {
                    m_p1->addRoundWin();
                    m_roundWinner = 1;
                } else {
                    // Time out or mutual KO: highest health wins
                    if (m_p1->getHealth() > m_p2->getHealth()) {
                        m_p1->addRoundWin();
                        m_roundWinner = 1;
                    } else {
                        m_p2->addRoundWin();
                        m_roundWinner = 2;
                    }
                }

                // Super dramatic KO slow-mo & camera zoom!
                timeManager.triggerSlowMo(0.08f, 2.2f);
                camera.triggerCinematicZoom(0.62f, 2.2f, (p1Pos.x < p2Pos.x ? -3.0f : 3.0f));
                camera.addTrauma(0.85f);
                juiceFX.triggerScreenFlash(0.16f, sf::Color(255, 255, 255, 175));
                sf::Vector2f koCenter = (p1Pos + p2Pos) * 0.5f + sf::Vector2f(0.0f, -40.0f);
                juiceFX.spawnFloatingText(koCenter, "K.O.!", sf::Color(255, 220, 40), 2.4f);
            }

            // 3. Dynamic Slow-Mo Clash Engine (Tekken 7/8 Style)
            if (m_clashCooldown > 0.0f) {
                m_clashCooldown -= dt;
            } else {
                checkSlowMoClash(timeManager, camera, juiceFX);
            }

            // 4. Hit Detection & Damage Resolution
            processStrikes(*m_p1, *m_p2, timeManager, camera, juiceFX, stageRenderer);
            processStrikes(*m_p2, *m_p1, timeManager, camera, juiceFX, stageRenderer);
        } else if (m_state == MatchState::RoundKO) {
            m_stateTimer -= dt;
            if (m_stateTimer <= 0.0f) {
                if (m_p1->getRoundsWon() >= 2 || m_p2->getRoundsWon() >= 2) {
                    m_state = MatchState::MatchOver;
                } else {
                    // Next round
                    m_p1->respawn(sf::Vector2f(550.0f, 735.0f));
                    m_p2->respawn(sf::Vector2f(1050.0f, 735.0f));
                    startRound(m_roundNumber + 1);
                }
            }
        }
    }

    MatchState getState() const { return m_state; }
    float getRoundTimer() const { return m_roundTimer; }
    int getRoundNumber() const { return m_roundNumber; }
    int getRoundWinner() const { return m_roundWinner; }

private:
    Fighter* m_p1{ nullptr };
    Fighter* m_p2{ nullptr };

    MatchState m_state{ MatchState::RoundIntro };
    int m_roundNumber{ 1 };
    float m_roundTimer{ 60.0f };
    float m_stateTimer{ 0.0f };
    int m_roundWinner{ 0 };

    float m_clashCooldown{ 0.0f };

    void checkSlowMoClash(RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX) {
        if (!m_p1->getController()->isAttacking() || !m_p2->getController()->isAttacking()) return;

        sf::Vector2f p1Pos = m_p1->getSkeleton()->getPositionPixels();
        sf::Vector2f p2Pos = m_p2->getSkeleton()->getPositionPixels();
        float dx = p2Pos.x - p1Pos.x;
        float dy = p2Pos.y - p1Pos.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        // When close and both unleash strikes simultaneously
        if (dist < 125.0f && !m_p1->getController()->hasHitRegistered() && !m_p2->getController()->hasHitRegistered()) {
            float p1Prog = m_p1->getController()->getMoveProgress();
            float p2Prog = m_p2->getController()->getMoveProgress();

            if (p1Prog < 0.50f && p2Prog < 0.50f) {
                m_clashCooldown = 2.5f;

                timeManager.triggerSlowMo(0.08f, 1.1f);
                camera.triggerCinematicZoom(0.65f, 1.1f, (p1Pos.x < p2Pos.x ? -3.5f : 3.5f));
                camera.addTrauma(0.40f);

                sf::Vector2f mid = (p1Pos + p2Pos) * 0.5f + sf::Vector2f(0.0f, -50.0f);
                juiceFX.spawnFloatingText(mid, "CLASH!", sf::Color(255, 230, 60), 1.4f);
                juiceFX.triggerScreenFlash(0.06f, sf::Color(255, 255, 255, 110));
            }
        }
    }

    void processStrikes(Fighter& attacker, Fighter& defender, RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX, RagdollEngine::StageRenderer* stageRenderer = nullptr) {
        auto* atCtrl = attacker.getController();
        auto* defCtrl = defender.getController();
        if (!atCtrl->isAttackActive() || atCtrl->hasHitRegistered()) return;

        const auto& move = atCtrl->getCurrentMove();
        sf::Vector2f strikeTip = atCtrl->getActiveStrikingTipPixels();

        auto* defSkel = defender.getSkeleton();
        if (!defSkel) return;

        // Check distance to opponent hurtboxes (Head, Torso, Hips)
        b2BodyId head = defSkel->getBody(RagdollEngine::LimbType::Head);
        b2BodyId torso = defSkel->getBody(RagdollEngine::LimbType::Torso);
        b2BodyId hips = defSkel->getBody(RagdollEngine::LimbType::Hips);

        auto checkBodyContact = [&](b2BodyId body, float reachRadius) {
            if (!b2Body_IsValid(body)) return false;
            sf::Vector2f bodyPos = RagdollEngine::PhysicsUnits::toPixels(b2Body_GetPosition(body));
            float dx = bodyPos.x - strikeTip.x;
            float dy = bodyPos.y - strikeTip.y;
            return (std::sqrt(dx * dx + dy * dy) < reachRadius);
        };

        float reachRadius = move.isThrow ? 34.0f : 26.0f;
        bool hitConnected = checkBodyContact(head, reachRadius) ||
                            checkBodyContact(torso, reachRadius) ||
                            checkBodyContact(hips, reachRadius);

        if (hitConnected) {
            atCtrl->setHitRegistered(true);

            // 1. Calculate Base Damage with Tekken 7 Rage multiplier (+15% when HP <= 28%)
            float baseDamage = move.damage;
            if (attacker.isInRage()) {
                baseDamage *= 1.15f;
            }

            b2Vec2 impulse = { move.launchImpulse.x, move.launchImpulse.y };
            RagdollEngine::HitResult hitRes = defCtrl->takeHit(baseDamage, move.height, impulse, move.isLauncher, move.isTrip, move.isThrow);

            if (hitRes == RagdollEngine::HitResult::ThrowBroken) {
                // Command Throw broken by defender!
                juiceFX.spawnFloatingText(strikeTip + sf::Vector2f(0.0f, -40.0f), "THROW BREAK!", sf::Color(80, 220, 255), 1.5f);
                juiceFX.spawnBlockEffect(strikeTip);
                camera.addTrauma(0.35f);
                timeManager.triggerHitstop(0.10f);

                // Push fighters apart
                b2BodyId atTorso = attacker.getSkeleton()->getTorso();
                b2BodyId defTorso = defender.getSkeleton()->getTorso();
                if (b2Body_IsValid(atTorso)) b2Body_ApplyLinearImpulseToCenter(atTorso, b2Vec2{ -atCtrl->getFacingDirection() * 6.5f, 0.0f }, true);
                if (b2Body_IsValid(defTorso)) b2Body_ApplyLinearImpulseToCenter(defTorso, b2Vec2{ atCtrl->getFacingDirection() * 6.5f, 0.0f }, true);
            } else if (hitRes == RagdollEngine::HitResult::ThrowGrabbed) {
                // Command Throw connected!
                juiceFX.spawnFloatingText(strikeTip + sf::Vector2f(0.0f, -45.0f), "THROW!", sf::Color(255, 215, 0), 1.6f);
                defender.takeDamage(baseDamage);
                attacker.addComboHit(baseDamage);
                camera.addTrauma(0.75f);
                timeManager.triggerHitstop(0.12f);
                juiceFX.spawnImpact(strikeTip, sf::Vector2f(0.0f, 1.0f), sf::Color(255, 215, 0), true);
            } else if (hitRes == RagdollEngine::HitResult::PowerCrushAbsorb) {
                // Power Crush Armor absorbed the hit! (50% white damage, no hitstun)
                float absorbedDmg = baseDamage * 0.5f;
                defender.takeDamage(absorbedDmg);
                juiceFX.spawnFloatingText(strikeTip + sf::Vector2f(0.0f, -40.0f), "POWER CRUSH!", sf::Color(255, 120, 30), 1.4f);
                juiceFX.spawnImpact(strikeTip, sf::Vector2f(atCtrl->getFacingDirection() * 1.0f, -0.2f), sf::Color(255, 120, 30), false);
                camera.addTrauma(0.35f);
                timeManager.triggerHitstop(0.06f);
            } else if (hitRes == RagdollEngine::HitResult::Blocked) {
                juiceFX.spawnBlockEffect(strikeTip);
                juiceFX.spawnFloatingText(strikeTip, "BLOCKED!", sf::Color(110, 210, 255), 1.0f);
                camera.addTrauma(0.18f);
                timeManager.triggerHitstop(0.05f);
            } else if (hitRes == RagdollEngine::HitResult::CleanHit || hitRes == RagdollEngine::HitResult::CounterHit) {
                bool isCounter = (hitRes == RagdollEngine::HitResult::CounterHit);
                float damageMult = isCounter ? 1.45f : 1.0f;
                float finalDamage = baseDamage * damageMult;

                defender.takeDamage(finalDamage);
                attacker.addComboHit(finalDamage);

                // Tekken 7 Rage Art Cinematic Sequence!
                if (move.isRageArt) {
                    attacker.setRageArtUsed(true);
                    juiceFX.spawnFloatingText(strikeTip + sf::Vector2f(0.0f, -50.0f), "RAGE ART!", sf::Color(255, 30, 30), 2.2f);
                    juiceFX.triggerScreenFlash(0.20f, sf::Color(255, 40, 40, 180));
                    camera.triggerCinematicZoom(0.50f, 1.8f, 0.0f);
                    timeManager.triggerSlowMo(0.06f, 1.6f);
                    camera.addTrauma(0.95f);
                }

                // Tekken 7 Wall Splat System:
                sf::Vector2f defPos = defender.getSkeleton()->getPositionPixels();
                if ((defPos.x < 190.0f || defPos.x > 1410.0f) &&
                    (std::abs(move.launchImpulse.x) > 7.0f || isCounter || move.isLauncher || move.isPowerCrush)) {
                    juiceFX.spawnFloatingText(defPos + sf::Vector2f(0.0f, -60.0f), "WALL SPLAT!", sf::Color(255, 160, 20), 1.6f);
                    defender.takeDamage(8.0f); // Bonus wall splat damage
                    camera.addTrauma(0.65f);
                    timeManager.triggerHitstop(0.14f);
                    juiceFX.spawnImpact(defPos, sf::Vector2f((defPos.x < 800.0f ? 1.0f : -1.0f), 0.0f), sf::Color(255, 160, 20), true);

                    if (stageRenderer) {
                        float wallX = (defPos.x < 800.0f) ? 60.0f : 1540.0f;
                        stageRenderer->addWallCrack(sf::Vector2f(wallX, defPos.y), 1.25f);
                        stageRenderer->spawnGroundDust(sf::Vector2f(wallX, defPos.y), sf::Vector2f((defPos.x < 800.0f ? 80.0f : -80.0f), -35.0f), 8);
                    }

                    // Wall stick: stop horizontal velocity momentarily for wall combo follow-up
                    b2BodyId defHips = defender.getSkeleton()->getHips();
                    if (b2Body_IsValid(defHips)) {
                        b2Vec2 vel = b2Body_GetLinearVelocity(defHips);
                        b2Body_SetLinearVelocity(defHips, b2Vec2{ 0.0f, std::min(vel.y, 1.0f) });
                    }
                }

                // Check Juggle Pop-Up
                if (defCtrl->getActionState() == RagdollEngine::FighterActionState::LaunchedJuggle) {
                    juiceFX.spawnFloatingText(strikeTip + sf::Vector2f(0.0f, -25.0f), "AIR JUGGLE!", sf::Color(255, 210, 60), 1.15f);
                    defCtrl->popUpJuggle(b2Vec2{ atCtrl->getFacingDirection() * 2.5f, -9.5f });
                }

                // Counter-Hit Text & Slow-Mo
                if (isCounter && !move.isRageArt) {
                    juiceFX.spawnFloatingText(strikeTip + sf::Vector2f(0.0f, -40.0f), "COUNTER HIT!", sf::Color(255, 60, 60), 1.4f);
                    timeManager.triggerHitstop(0.13f);
                    timeManager.triggerSlowMo(0.12f, 0.70f);
                    camera.addTrauma(0.70f);
                    juiceFX.triggerScreenFlash(0.10f, sf::Color(255, 255, 255, 140));
                } else if (!move.isRageArt) {
                    timeManager.triggerHitstop(move.isLauncher ? 0.10f : 0.07f);
                    if (move.isLauncher) {
                        timeManager.triggerSlowMo(0.16f, 0.75f);
                    }
                    camera.addTrauma(move.isLauncher ? 0.60f : 0.40f);
                }

                // Spiky Starburst & sparks
                sf::Color sparkColor = move.isElectric ? sf::Color(120, 220, 255)
                                     : move.isRageArt ? sf::Color(255, 40, 40)
                                     : isCounter ? sf::Color(255, 70, 70)
                                     : sf::Color(255, 230, 80);
                juiceFX.spawnImpact(strikeTip, sf::Vector2f(atCtrl->getFacingDirection() * 1.0f, -0.4f), sparkColor, move.isLauncher || isCounter || move.isRageArt);
            }
        }
    }
};

} // namespace StickminGame
