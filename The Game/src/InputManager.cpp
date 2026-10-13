#include "InputManager.hpp"
#include <cmath>

namespace StickminGame {

PlayerKeyBindings PlayerKeyBindings::defaultP1() {
    PlayerKeyBindings b;
    b.up = sf::Keyboard::Key::W;
    b.down = sf::Keyboard::Key::S;
    b.left = sf::Keyboard::Key::A;
    b.right = sf::Keyboard::Key::D;
    b.jumpAlt = sf::Keyboard::Key::Space;

    b.lp = sf::Keyboard::Key::J;
    b.rp = sf::Keyboard::Key::K;
    b.lk = sf::Keyboard::Key::U;
    b.rk = sf::Keyboard::Key::I;
    b.powerCrush = sf::Keyboard::Key::O;
    b.throwKey = sf::Keyboard::Key::L;
    b.dropkick = sf::Keyboard::Key::P;

    // Alt keys for arrow support (e.g. online client mode)
    b.altLeft = sf::Keyboard::Key::Left;
    b.altRight = sf::Keyboard::Key::Right;
    b.altDown = sf::Keyboard::Key::Down;
    b.altUp = sf::Keyboard::Key::Up;
    return b;
}

PlayerKeyBindings PlayerKeyBindings::defaultP2() {
    PlayerKeyBindings b;
    b.up = sf::Keyboard::Key::Up;
    b.down = sf::Keyboard::Key::Down;
    b.left = sf::Keyboard::Key::Left;
    b.right = sf::Keyboard::Key::Right;
    b.jumpAlt = sf::Keyboard::Key::Numpad0;

    b.lp = sf::Keyboard::Key::Numpad1;
    b.altLp = sf::Keyboard::Key::Comma;

    b.rp = sf::Keyboard::Key::Numpad2;
    b.altRp = sf::Keyboard::Key::Period;

    b.lk = sf::Keyboard::Key::Numpad4;
    b.altLk = sf::Keyboard::Key::Slash;

    b.rk = sf::Keyboard::Key::Numpad5;
    b.altRk = sf::Keyboard::Key::Semicolon;

    b.powerCrush = sf::Keyboard::Key::Numpad3;
    b.altPowerCrush = sf::Keyboard::Key::RBracket;

    b.throwKey = sf::Keyboard::Key::Numpad6;
    b.altThrow = sf::Keyboard::Key::Apostrophe;

    b.dropkick = sf::Keyboard::Key::Numpad9;
    b.altDropkick = sf::Keyboard::Key::LBracket;
    return b;
}

InputManager::InputManager()
    : m_p1Bindings(PlayerKeyBindings::defaultP1()),
      m_p2Bindings(PlayerKeyBindings::defaultP2()) {
}

bool InputManager::isKeyPressed(sf::Keyboard::Key primary, sf::Keyboard::Key secondary) const {
    if (primary != sf::Keyboard::Key::Unknown && sf::Keyboard::isKeyPressed(primary)) return true;
    if (secondary != sf::Keyboard::Key::Unknown && sf::Keyboard::isKeyPressed(secondary)) return true;
    return false;
}

void InputManager::reset() {
    m_p1RightDown = false;
    m_p1LeftDown = false;
    m_p1LastRightReleaseTime = -10.0f;
    m_p1LastLeftReleaseTime = -10.0f;
    m_p1PendingDash = 0;
    m_p1PendingButtons = 0;

    m_p2RightDown = false;
    m_p2LeftDown = false;
    m_p2LastRightReleaseTime = -10.0f;
    m_p2LastLeftReleaseTime = -10.0f;
    m_p2PendingDash = 0;
    m_p2PendingButtons = 0;
}

void InputManager::handleKeyPressed(sf::Keyboard::Key key, float gameTime, bool allowP1Arrows) {
    // -------------------------------------------------------------------------
    // Player 1 Discrete Actions
    // -------------------------------------------------------------------------
    if (key == m_p1Bindings.up || key == m_p1Bindings.jumpAlt || (allowP1Arrows && key == m_p1Bindings.altUp)) {
        m_p1PendingButtons |= InputButtons::Jump;
    }

    // 1: LP (Check simultaneous RP -> PowerCrush, or LK -> Throw)
    if (key == m_p1Bindings.lp) {
        if (isKeyPressed(m_p1Bindings.rp)) {
            m_p1PendingButtons |= InputButtons::PowerCrush;
        } else if (isKeyPressed(m_p1Bindings.lk)) {
            m_p1PendingButtons |= InputButtons::Throw;
        } else {
            m_p1PendingButtons |= InputButtons::LP;
        }
    }
    // 2: RP (Check simultaneous LP -> PowerCrush)
    if (key == m_p1Bindings.rp) {
        if (isKeyPressed(m_p1Bindings.lp)) {
            m_p1PendingButtons |= InputButtons::PowerCrush;
        } else {
            m_p1PendingButtons |= InputButtons::RP;
        }
    }
    // 3: LK (Check simultaneous LP -> Throw)
    if (key == m_p1Bindings.lk) {
        if (isKeyPressed(m_p1Bindings.lp)) {
            m_p1PendingButtons |= InputButtons::Throw;
        } else {
            m_p1PendingButtons |= InputButtons::LK;
        }
    }
    // 4: RK
    if (key == m_p1Bindings.rk) {
        m_p1PendingButtons |= InputButtons::RK;
    }
    // 1+2: PowerCrush
    if (key == m_p1Bindings.powerCrush) {
        m_p1PendingButtons |= InputButtons::PowerCrush;
    }
    // 1+3: Throw
    if (key == m_p1Bindings.throwKey) {
        m_p1PendingButtons |= InputButtons::Throw;
    }
    // 3+4: Dropkick
    if (key == m_p1Bindings.dropkick) {
        m_p1PendingButtons |= InputButtons::Dropkick;
    }

    // Double-tap dash detection P1 (Right / Forward)
    if (key == m_p1Bindings.right || (allowP1Arrows && key == m_p1Bindings.altRight)) {
        if (!m_p1RightDown) {
            m_p1RightDown = true;
            if (gameTime - m_p1LastRightReleaseTime < 0.22f) {
                m_p1PendingDash = 1;
            }
        }
    }
    // Double-tap dash detection P1 (Left / Back)
    if (key == m_p1Bindings.left || (allowP1Arrows && key == m_p1Bindings.altLeft)) {
        if (!m_p1LeftDown) {
            m_p1LeftDown = true;
            if (gameTime - m_p1LastLeftReleaseTime < 0.22f) {
                m_p1PendingDash = -1;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Player 2 Discrete Actions (Disabled when P1 is using arrow keys, e.g. Online)
    // -------------------------------------------------------------------------
    if (!allowP1Arrows) {
        if (key == m_p2Bindings.up || key == m_p2Bindings.jumpAlt) {
            m_p2PendingButtons |= InputButtons::Jump;
        }

        // 1: LP
        if (key == m_p2Bindings.lp || key == m_p2Bindings.altLp) {
            if (isKeyPressed(m_p2Bindings.rp, m_p2Bindings.altRp)) {
                m_p2PendingButtons |= InputButtons::PowerCrush;
            } else if (isKeyPressed(m_p2Bindings.lk, m_p2Bindings.altLk)) {
                m_p2PendingButtons |= InputButtons::Throw;
            } else {
                m_p2PendingButtons |= InputButtons::LP;
            }
        }
        // 2: RP
        if (key == m_p2Bindings.rp || key == m_p2Bindings.altRp) {
            if (isKeyPressed(m_p2Bindings.lp, m_p2Bindings.altLp)) {
                m_p2PendingButtons |= InputButtons::PowerCrush;
            } else {
                m_p2PendingButtons |= InputButtons::RP;
            }
        }
        // 3: LK
        if (key == m_p2Bindings.lk || key == m_p2Bindings.altLk) {
            if (isKeyPressed(m_p2Bindings.lp, m_p2Bindings.altLp)) {
                m_p2PendingButtons |= InputButtons::Throw;
            } else {
                m_p2PendingButtons |= InputButtons::LK;
            }
        }
        // 4: RK
        if (key == m_p2Bindings.rk || key == m_p2Bindings.altRk) {
            m_p2PendingButtons |= InputButtons::RK;
        }
        // 1+2: PowerCrush
        if (key == m_p2Bindings.powerCrush || key == m_p2Bindings.altPowerCrush) {
            m_p2PendingButtons |= InputButtons::PowerCrush;
        }
        // 1+3: Throw
        if (key == m_p2Bindings.throwKey || key == m_p2Bindings.altThrow) {
            m_p2PendingButtons |= InputButtons::Throw;
        }
        // 3+4: Dropkick
        if (key == m_p2Bindings.dropkick || key == m_p2Bindings.altDropkick) {
            m_p2PendingButtons |= InputButtons::Dropkick;
        }

        // Double-tap dash detection P2 (Right)
        if (key == m_p2Bindings.right) {
            if (!m_p2RightDown) {
                m_p2RightDown = true;
                if (gameTime - m_p2LastRightReleaseTime < 0.22f) {
                    m_p2PendingDash = 1;
                }
            }
        }
        // Double-tap dash detection P2 (Left)
        if (key == m_p2Bindings.left) {
            if (!m_p2LeftDown) {
                m_p2LeftDown = true;
                if (gameTime - m_p2LastLeftReleaseTime < 0.22f) {
                    m_p2PendingDash = -1;
                }
            }
        }
    }
}

void InputManager::handleKeyReleased(sf::Keyboard::Key key, float gameTime, bool allowP1Arrows) {
    if (key == m_p1Bindings.right || (allowP1Arrows && key == m_p1Bindings.altRight)) {
        m_p1RightDown = false;
        m_p1LastRightReleaseTime = gameTime;
    }
    if (key == m_p1Bindings.left || (allowP1Arrows && key == m_p1Bindings.altLeft)) {
        m_p1LeftDown = false;
        m_p1LastLeftReleaseTime = gameTime;
    }

    if (!allowP1Arrows) {
        if (key == m_p2Bindings.right) {
            m_p2RightDown = false;
            m_p2LastRightReleaseTime = gameTime;
        }
        if (key == m_p2Bindings.left) {
            m_p2LeftDown = false;
            m_p2LastLeftReleaseTime = gameTime;
        }
    }
}

uint16_t InputManager::consumeP1Buttons() {
    uint16_t b = m_p1PendingButtons;
    m_p1PendingButtons = 0;
    return b;
}

int8_t InputManager::consumeP1Dash() {
    int8_t d = m_p1PendingDash;
    m_p1PendingDash = 0;
    return d;
}

uint16_t InputManager::consumeP2Buttons() {
    uint16_t b = m_p2PendingButtons;
    m_p2PendingButtons = 0;
    return b;
}

int8_t InputManager::consumeP2Dash() {
    int8_t d = m_p2PendingDash;
    m_p2PendingDash = 0;
    return d;
}

ClientInputData InputManager::collectP1Input(bool hasFocus, bool allowP1Arrows) {
    ClientInputData input;
    if (hasFocus) {
        if (isKeyPressed(m_p1Bindings.left, allowP1Arrows ? m_p1Bindings.altLeft : sf::Keyboard::Key::Unknown)) {
            input.moveX -= 1.0f;
        }
        if (isKeyPressed(m_p1Bindings.right, allowP1Arrows ? m_p1Bindings.altRight : sf::Keyboard::Key::Unknown)) {
            input.moveX += 1.0f;
        }
        if (isKeyPressed(m_p1Bindings.down, allowP1Arrows ? m_p1Bindings.altDown : sf::Keyboard::Key::Unknown)) {
            input.moveY += 1.0f;
        }
    }
    input.buttons = consumeP1Buttons();
    input.dashDir = consumeP1Dash();
    return input;
}

ClientInputData InputManager::collectP2Input(bool hasFocus) {
    ClientInputData input;
    if (hasFocus) {
        if (isKeyPressed(m_p2Bindings.left)) {
            input.moveX -= 1.0f;
        }
        if (isKeyPressed(m_p2Bindings.right)) {
            input.moveX += 1.0f;
        }
        if (isKeyPressed(m_p2Bindings.down)) {
            input.moveY += 1.0f;
        }
    }
    input.buttons = consumeP2Buttons();
    input.dashDir = consumeP2Dash();
    return input;
}

void InputManager::applyInputToFighter(
    const ClientInputData& input,
    Fighter& fighter,
    float gameTime,
    float& last1Time,
    RagdollEngine::StageRenderer* stageRenderer) {
    
    auto* ctrl = fighter.getController();
    if (!ctrl) return;

    // 1. Continuous movement
    ctrl->setMoveInput(input.moveX, input.moveY);

    // 2. Dash execution
    if (input.dashDir != 0) {
        ctrl->triggerDash(input.dashDir);
        if (stageRenderer) {
            stageRenderer->spawnGroundDust(
                fighter.getSkeleton()->getPositionPixels() + sf::Vector2f(0.0f, 65.0f),
                sf::Vector2f(-input.dashDir * 110.0f, -20.0f),
                6
            );
        }
    }

    // 3. Jump execution
    if (input.buttons & InputButtons::Jump) {
        ctrl->jump();
        if (stageRenderer) {
            stageRenderer->spawnGroundDust(
                fighter.getSkeleton()->getPositionPixels() + sf::Vector2f(0.0f, 65.0f),
                sf::Vector2f(0.0f, -40.0f),
                8
            );
        }
    }

    // 4. Attack Moves Execution
    bool fwd = (ctrl->getFacingDirection() > 0) ? (input.moveX > 0) : (input.moveX < 0);
    bool down = (input.moveY > 0);
    bool up = (input.buttons & InputButtons::Jump);

    if (input.buttons & InputButtons::PowerCrush) {
        if (fighter.isInRage() && !fighter.hasUsedRageArt()) {
            ctrl->triggerMove(RagdollEngine::MoveId::RageArt);
        } else {
            ctrl->triggerMove(RagdollEngine::MoveId::PowerCrush);
        }
    } else if (input.buttons & InputButtons::Throw) {
        ctrl->triggerMove(RagdollEngine::MoveId::Throw);
    } else if (input.buttons & InputButtons::Dropkick) {
        ctrl->triggerMove(RagdollEngine::MoveId::FlyingDropkick);
    } else if (input.buttons & InputButtons::LP) {
        last1Time = gameTime;
        ctrl->triggerMove(RagdollEngine::MoveId::FlashJab);
    } else if (input.buttons & InputButtons::RP) {
        if (fwd) {
            ctrl->triggerMove(RagdollEngine::MoveId::ElectricWindGodFist);
        } else if (gameTime - last1Time < 0.28f) {
            ctrl->triggerMove(RagdollEngine::MoveId::OneTwoString);
        } else {
            ctrl->triggerMove(RagdollEngine::MoveId::StraightCross);
        }
    } else if (input.buttons & InputButtons::LK) {
        if (down) {
            ctrl->triggerMove(RagdollEngine::MoveId::HellSweep);
        } else {
            ctrl->triggerMove(RagdollEngine::MoveId::MidKick);
        }
    } else if (input.buttons & InputButtons::RK) {
        if (up) {
            ctrl->triggerMove(RagdollEngine::MoveId::Hopkick);
        } else {
            ctrl->triggerMove(RagdollEngine::MoveId::AxeRoundhouse);
        }
    }
}

} // namespace StickminGame
