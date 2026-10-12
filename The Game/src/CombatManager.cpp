#include "CombatManager.hpp"
#include <RagdollEngine/Physics/PhysicsUnits.hpp>
#include <cmath>
#include <algorithm>

namespace StickminGame {

CombatManager::CombatManager(Fighter* p1, Fighter* p2)
    : m_p1(p1), m_p2(p2) {
}

void CombatManager::startRound(int roundNumber) {
    m_roundNumber = roundNumber;
    m_roundTimer = 60.0f;
    m_state = MatchState::RoundIntro;
    m_stateTimer = 1.6f;
    m_clashCooldown = 0.0f;
}

void CombatManager::update(float dt, RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX, RagdollEngine::StageRenderer* stageRenderer) {
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

            // KO sequence
            timeManager.triggerSlowMo(0.08f, 2.2f);
            camera.triggerCinematicZoom(0.32f, 2.2f, (p1Pos.x < p2Pos.x ? -3.0f : 3.0f));
            camera.addTrauma(0.85f);
            juiceFX.triggerScreenFlash(0.16f, sf::Color(255, 255, 255, 175));
            queueJuiceEvent(JuiceEventType::ScreenFlash, sf::Vector2f(0.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 255, 255, 175), 0.16f);
        }

        // 3. Simultaneous attack clash check
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
                // Next round (Tekken starting distance)
                m_p1->respawn(sf::Vector2f(650.0f, 735.0f));
                m_p2->respawn(sf::Vector2f(950.0f, 735.0f));
                startRound(m_roundNumber + 1);
            }
        }
    }
}

std::vector<JuiceFXEvent> CombatManager::consumePendingJuiceEvents() {
    std::vector<JuiceFXEvent> evs = std::move(m_pendingJuiceEvents);
    m_pendingJuiceEvents.clear();
    return evs;
}

void CombatManager::queueJuiceEvent(JuiceEventType type, sf::Vector2f pos, sf::Vector2f normal, sf::Color col, float intensity, const std::string& text) {
    JuiceFXEvent ev;
    ev.type = type;
    ev.posX = pos.x;
    ev.posY = pos.y;
    ev.normalX = normal.x;
    ev.normalY = normal.y;
    ev.colorR = col.r;
    ev.colorG = col.g;
    ev.colorB = col.b;
    ev.intensity = intensity;
    ev.text = text;
    m_pendingJuiceEvents.push_back(std::move(ev));
}

void CombatManager::checkSlowMoClash(RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX) {
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
            camera.triggerCinematicZoom(0.34f, 1.1f, (p1Pos.x < p2Pos.x ? -3.5f : 3.5f));
            camera.addTrauma(0.40f);

            sf::Vector2f mid = (p1Pos + p2Pos) * 0.5f + sf::Vector2f(0.0f, -50.0f);
            m_p1->triggerAlert("CLASH", sf::Color(255, 230, 60), 1.2f);
            m_p2->triggerAlert("CLASH", sf::Color(255, 230, 60), 1.2f);
            juiceFX.triggerScreenFlash(0.06f, sf::Color(255, 255, 255, 110));
            queueJuiceEvent(JuiceEventType::FloatingText, sf::Vector2f(800.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 230, 60), 1.2f, "CLASH");
            queueJuiceEvent(JuiceEventType::ScreenFlash, sf::Vector2f(0.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 255, 255, 110), 0.06f);
        }
    }
}

void CombatManager::processStrikes(Fighter& attacker, Fighter& defender, RagdollEngine::TimeManager& timeManager, RagdollEngine::CameraDirector& camera, RagdollEngine::JuiceFX& juiceFX, RagdollEngine::StageRenderer* stageRenderer) {
    auto* atCtrl = attacker.getController();
    auto* defCtrl = defender.getController();
    if (!atCtrl || !defCtrl) return;

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
        RagdollEngine::HitResult hitRes = defCtrl->takeHit(baseDamage, move.height, impulse, move.isLauncher, move.isTrip, move.isThrow, move.isCounterLauncher);

        if (hitRes == RagdollEngine::HitResult::ThrowBroken) {
            // Throw broken by defender
            defender.triggerAlert("THROW BREAK", sf::Color(80, 220, 255), 1.2f);
            juiceFX.spawnBlockEffect(strikeTip);
            camera.addTrauma(0.35f);
            timeManager.triggerHitstop(0.10f);
            queueJuiceEvent(JuiceEventType::FloatingText, sf::Vector2f(defender.getId() == 1 ? 100.0f : 1500.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(80, 220, 255), 1.2f, "THROW BREAK");
            queueJuiceEvent(JuiceEventType::ImpactHit, strikeTip, sf::Vector2f(0.0f, -1.0f), sf::Color(80, 220, 255), 1.0f);

            // Push fighters apart
            b2BodyId atTorso = attacker.getSkeleton()->getTorso();
            b2BodyId defTorso = defender.getSkeleton()->getTorso();
            if (b2Body_IsValid(atTorso)) b2Body_ApplyLinearImpulseToCenter(atTorso, b2Vec2{ -atCtrl->getFacingDirection() * 6.5f, 0.0f }, true);
            if (b2Body_IsValid(defTorso)) b2Body_ApplyLinearImpulseToCenter(defTorso, b2Vec2{ atCtrl->getFacingDirection() * 6.5f, 0.0f }, true);
        } else if (hitRes == RagdollEngine::HitResult::ThrowGrabbed) {
            // Throw connected
            defender.takeDamage(baseDamage);
            attacker.addComboHit(baseDamage);
            camera.addTrauma(0.75f);
            timeManager.triggerHitstop(0.12f);
            juiceFX.spawnImpact(strikeTip, sf::Vector2f(0.0f, 1.0f), sf::Color(255, 215, 0), true);
            queueJuiceEvent(JuiceEventType::ImpactHit, strikeTip, sf::Vector2f(0.0f, 1.0f), sf::Color(255, 215, 0), 1.0f);
        } else if (hitRes == RagdollEngine::HitResult::PowerCrushAbsorb) {
            // Power Crush armor absorbed hit
            float absorbedDmg = baseDamage * 0.5f;
            defender.takeDamage(absorbedDmg);
            defender.triggerAlert("POWER CRUSH", sf::Color(255, 120, 30), 1.2f);
            juiceFX.spawnImpact(strikeTip, sf::Vector2f(atCtrl->getFacingDirection() * 1.0f, -0.2f), sf::Color(255, 120, 30), false);
            camera.addTrauma(0.35f);
            timeManager.triggerHitstop(0.06f);
            queueJuiceEvent(JuiceEventType::FloatingText, sf::Vector2f(defender.getId() == 1 ? 100.0f : 1500.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 120, 30), 1.2f, "POWER CRUSH");
            queueJuiceEvent(JuiceEventType::ImpactHit, strikeTip, sf::Vector2f(atCtrl->getFacingDirection() * 1.0f, -0.2f), sf::Color(255, 120, 30), 1.0f);
        } else if (hitRes == RagdollEngine::HitResult::Blocked) {
            juiceFX.spawnBlockEffect(strikeTip);
            camera.addTrauma(0.18f);
            timeManager.triggerHitstop(0.05f);
            queueJuiceEvent(JuiceEventType::ImpactHit, strikeTip, sf::Vector2f(0.0f, -1.0f), sf::Color(110, 210, 255), 1.0f);
        } else if (hitRes == RagdollEngine::HitResult::CleanHit || hitRes == RagdollEngine::HitResult::CounterHit) {
            bool isCounter = (hitRes == RagdollEngine::HitResult::CounterHit);
            bool isPunish = (!isCounter && defCtrl->isAttackRecovery());
            float damageMult = isCounter ? 1.45f : 1.0f;
            float finalDamage = baseDamage * damageMult;

            defender.takeDamage(finalDamage);
            attacker.addComboHit(finalDamage);

            // Technical Combat Alerts (Tekken 7 HUD cues)
            if (isCounter) {
                attacker.triggerAlert("COUNTER HIT", sf::Color(255, 50, 50), 1.2f);
                queueJuiceEvent(JuiceEventType::FloatingText, sf::Vector2f(attacker.getId() == 1 ? 100.0f : 1500.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 50, 50), 1.2f, "COUNTER HIT");
            } else if (isPunish) {
                attacker.triggerAlert("PUNISH", sf::Color(255, 215, 60), 1.2f);
                queueJuiceEvent(JuiceEventType::FloatingText, sf::Vector2f(attacker.getId() == 1 ? 100.0f : 1500.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 215, 60), 1.2f, "PUNISH");
            }

            // Rage Art Sequence
            if (move.isRageArt) {
                attacker.setRageArtUsed(true);
                attacker.triggerAlert("RAGE ART", sf::Color(255, 30, 30), 1.8f);
                juiceFX.triggerScreenFlash(0.20f, sf::Color(255, 40, 40, 180));
                camera.triggerCinematicZoom(0.28f, 1.8f, 0.0f);
                timeManager.triggerSlowMo(0.06f, 1.6f);
                camera.addTrauma(0.95f);
                queueJuiceEvent(JuiceEventType::FloatingText, sf::Vector2f(attacker.getId() == 1 ? 100.0f : 1500.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 30, 30), 1.8f, "RAGE ART");
                queueJuiceEvent(JuiceEventType::ScreenFlash, sf::Vector2f(0.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 40, 40, 180), 0.20f);
            }

            // Wall Splat System
            sf::Vector2f defPos = defender.getSkeleton()->getPositionPixels();
            if ((defPos.x < 190.0f || defPos.x > 1410.0f) &&
                (std::abs(move.launchImpulse.x) > 7.0f || isCounter || move.isLauncher || move.isPowerCrush)) {
                attacker.triggerAlert("WALL SPLAT", sf::Color(255, 160, 20), 1.3f);
                defender.takeDamage(8.0f); // Bonus wall splat damage
                camera.addTrauma(0.65f);
                timeManager.triggerHitstop(0.14f);
                juiceFX.spawnImpact(defPos, sf::Vector2f((defPos.x < 800.0f ? 1.0f : -1.0f), 0.0f), sf::Color(255, 160, 20), true);
                queueJuiceEvent(JuiceEventType::FloatingText, sf::Vector2f(attacker.getId() == 1 ? 100.0f : 1500.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 160, 20), 1.3f, "WALL SPLAT");
                queueJuiceEvent(JuiceEventType::ImpactHit, defPos, sf::Vector2f((defPos.x < 800.0f ? 1.0f : -1.0f), 0.0f), sf::Color(255, 160, 20), 1.0f);

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
                defCtrl->popUpJuggle(b2Vec2{ atCtrl->getFacingDirection() * 2.5f, -9.5f });
            }

            // Counter-Hit Slow-Mo
            if (isCounter && !move.isRageArt) {
                timeManager.triggerHitstop(0.13f);
                timeManager.triggerSlowMo(0.12f, 0.70f);
                camera.addTrauma(0.70f);
                juiceFX.triggerScreenFlash(0.10f, sf::Color(255, 255, 255, 140));
                queueJuiceEvent(JuiceEventType::ScreenFlash, sf::Vector2f(0.0f, 0.0f), sf::Vector2f(0.0f, 0.0f), sf::Color(255, 255, 255, 140), 0.10f);
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
            if (move.isElectric) {
                queueJuiceEvent(JuiceEventType::ElectricBurst, strikeTip, sf::Vector2f(0.0f, 0.0f), sf::Color(120, 220, 255), 1.0f);
            } else {
                queueJuiceEvent(JuiceEventType::ImpactHit, strikeTip, sf::Vector2f(atCtrl->getFacingDirection() * 1.0f, -0.4f), sparkColor, 1.0f);
            }
        }
    }
}

} // namespace StickminGame
