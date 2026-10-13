#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <SFML/System/Vector2.hpp>
#include "NetworkProtocol.hpp"
#include "Fighter.hpp"

#include <RagdollEngine/Render/StageRenderer.hpp>

namespace StickminGame {

struct PlayerKeyBindings {
    sf::Keyboard::Key up{ sf::Keyboard::Key::W };
    sf::Keyboard::Key down{ sf::Keyboard::Key::S };
    sf::Keyboard::Key left{ sf::Keyboard::Key::A };
    sf::Keyboard::Key right{ sf::Keyboard::Key::D };
    sf::Keyboard::Key jumpAlt{ sf::Keyboard::Key::Space };

    sf::Keyboard::Key lp{ sf::Keyboard::Key::J };        // 1
    sf::Keyboard::Key rp{ sf::Keyboard::Key::K };        // 2
    sf::Keyboard::Key lk{ sf::Keyboard::Key::U };        // 3
    sf::Keyboard::Key rk{ sf::Keyboard::Key::I };        // 4
    sf::Keyboard::Key powerCrush{ sf::Keyboard::Key::O };// 1+2
    sf::Keyboard::Key throwKey{ sf::Keyboard::Key::L };  // 1+3
    sf::Keyboard::Key dropkick{ sf::Keyboard::Key::P };  // 3+4

    // Alternate keys (for secondary layout / arrows)
    sf::Keyboard::Key altLeft{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altRight{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altDown{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altUp{ sf::Keyboard::Key::Unknown };

    sf::Keyboard::Key altLp{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altRp{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altLk{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altRk{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altPowerCrush{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altThrow{ sf::Keyboard::Key::Unknown };
    sf::Keyboard::Key altDropkick{ sf::Keyboard::Key::Unknown };

    static PlayerKeyBindings defaultP1();
    static PlayerKeyBindings defaultP2();
};

class InputManager {
public:
    InputManager();

    void setP1Bindings(const PlayerKeyBindings& b) { m_p1Bindings = b; }
    void setP2Bindings(const PlayerKeyBindings& b) { m_p2Bindings = b; }
    const PlayerKeyBindings& getP1Bindings() const { return m_p1Bindings; }
    const PlayerKeyBindings& getP2Bindings() const { return m_p2Bindings; }

    // Event handlers for discrete combat buttons & double-tap dash timing
    void handleKeyPressed(sf::Keyboard::Key key, float gameTime, bool allowP1Arrows = false);
    void handleKeyReleased(sf::Keyboard::Key key, float gameTime, bool allowP1Arrows = false);

    // Collect instantaneous inputs for P1 and P2
    ClientInputData collectP1Input(bool hasFocus, bool allowP1Arrows = false);
    ClientInputData collectP2Input(bool hasFocus);

    // Consume pending one-shot triggers (buttons and dashes)
    uint16_t consumeP1Buttons();
    int8_t consumeP1Dash();
    uint16_t consumeP2Buttons();
    int8_t consumeP2Dash();

    // Unified command execution: translates abstract input actions into character moves
    static void applyInputToFighter(
        const ClientInputData& input,
        Fighter& fighter,
        float gameTime,
        float& last1Time,
        RagdollEngine::StageRenderer* stageRenderer = nullptr
    );

    void reset();

private:
    PlayerKeyBindings m_p1Bindings;
    PlayerKeyBindings m_p2Bindings;

    // P1 double-tap state
    bool m_p1RightDown{ false };
    bool m_p1LeftDown{ false };
    float m_p1LastRightReleaseTime{ -10.0f };
    float m_p1LastLeftReleaseTime{ -10.0f };
    int8_t m_p1PendingDash{ 0 };
    uint16_t m_p1PendingButtons{ 0 };

    // P2 double-tap state
    bool m_p2RightDown{ false };
    bool m_p2LeftDown{ false };
    float m_p2LastRightReleaseTime{ -10.0f };
    float m_p2LastLeftReleaseTime{ -10.0f };
    int8_t m_p2PendingDash{ 0 };
    uint16_t m_p2PendingButtons{ 0 };

    bool isKeyPressed(sf::Keyboard::Key primary, sf::Keyboard::Key secondary = sf::Keyboard::Key::Unknown) const;
};

} // namespace StickminGame
