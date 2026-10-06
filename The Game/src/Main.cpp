#include <SFML/Graphics.hpp>
#include <RagdollEngine/Core/TimeManager.hpp>
#include <RagdollEngine/Core/CameraDirector.hpp>
#include <RagdollEngine/Physics/PhysicsWorld.hpp>
#include <RagdollEngine/Physics/PhysicsUnits.hpp>
#include <RagdollEngine/Render/RagdollRenderer.hpp>
#include <RagdollEngine/Render/JuiceFX.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>

#include "Fighter.hpp"
#include "CombatManager.hpp"
#include "MenuManager.hpp"

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <string>

int main() {
    // 1. Create Window with SFML 3 VideoMode
    sf::ContextSettings settings;
    sf::RenderWindow window(
        sf::VideoMode({1600, 900}),
        "Stickmin Arena",
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );
    window.setFramerateLimit(144);

    // 2. Initialize Engine Subsystems
    RagdollEngine::TimeManager timeManager;
    RagdollEngine::CameraDirector camera(1600.0f, 900.0f);
    camera.setZoomLimits(0.42f, 1.05f); // Close, punchy Tekken framing with full arena coverage

    RagdollEngine::PhysicsWorld physicsWorld(19.0f); // Balanced gravity
    RagdollEngine::RagdollRenderer ragdollRenderer;
    RagdollEngine::JuiceFX juiceFX;
    RagdollEngine::StageRenderer stageRenderer;

    // Load HUD Font
    sf::Font hudFont;
    bool fontLoaded = hudFont.openFromFile("C:/Windows/Fonts/segoeui.ttf") ||
                      hudFont.openFromFile("C:/Windows/Fonts/arial.ttf");
    if (fontLoaded) {
        juiceFX.setFont(&hudFont);
    }

    StickminGame::MenuManager menuManager(fontLoaded ? &hudFont : nullptr, &ragdollRenderer, &stageRenderer);

    // 3. Build Arena Geometry (Tekken-style flat combat arena with left & right boundary walls)
    // Floor
    physicsWorld.createStaticBox(800.0f, 820.0f, 1500.0f, 40.0f, 0.95f);
    // Left & Right Arena Walls (Wall Splats)
    physicsWorld.createStaticBox(60.0f, 450.0f, 40.0f, 800.0f, 0.2f);
    physicsWorld.createStaticBox(1540.0f, 450.0f, 40.0f, 800.0f, 0.2f);

    // Dynamic props (empty on combat plane to maintain pure Tekken fighting flow)
    std::vector<b2BodyId> props;

    // 4. Spawn Fighters from CharacterRegistry (Starting at Tekken face-off distance)
    const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();
    size_t p1CharIdx = 0; // Henry Stickmin
    size_t p2CharIdx = 1; // Ellie Rose

    StickminGame::Fighter p1(physicsWorld.getB2WorldId(), 1, roster[p1CharIdx], sf::Vector2f(650.0f, 735.0f));
    StickminGame::Fighter p2(physicsWorld.getB2WorldId(), 2, roster[p2CharIdx], sf::Vector2f(950.0f, 735.0f));

    StickminGame::CombatManager combatManager(&p1, &p2);
    combatManager.startRound(1);

    float p1Last1Time = -10.0f;
    float p2Last1Time = -10.0f;

    bool p1DDown = false;
    bool p1ADown = false;
    float p1LastDReleaseTime = -10.0f;
    float p1LastAReleaseTime = -10.0f;

    bool p2RightDown = false;
    bool p2LeftDown = false;
    float p2LastRightReleaseTime = -10.0f;
    float p2LastLeftReleaseTime = -10.0f;

    std::cout << "====================================================\n";
    std::cout << " Stickmin Arena - Tekken 7 Active Ragdolls!\n";
    std::cout << " Roster: Henry, Ellie, Charles Calvin, Reginald, RHM!\n";
    std::cout << " [F1]: Cycle Player 1 Character\n";
    std::cout << " [F2]: Cycle Player 2 Character\n";
    std::cout << " [F5]: Cycle Stage Arena (Toppat Airship, The Wall, Bank Vault)\n";
    std::cout << " Player 1 (Tekken 7 4-Button Controls):\n";
    std::cout << "   A / D       : Move / Guard (Hold Back to Block!)\n";
    std::cout << "   S           : Crouch (Hold Down+Back for Crouch Block!)\n";
    std::cout << "   W / Space   : Jump\n";
    std::cout << "   J           : 1 (Left Punch - Flash Jab)\n";
    std::cout << "   K           : 2 (Right Punch - Straight Cross / Fwd+2: EWGF / 1,2: One-Two!)\n";
    std::cout << "   U           : 3 (Left Kick - Mid Kick / Down+3: Hell Sweep!)\n";
    std::cout << "   I           : 4 (Right Kick - Axe Roundhouse / Up+4: Hopkick!)\n";
    std::cout << "   O           : 1+2 (Power Crush Armor / Rage Art when in Rage!)\n";
    std::cout << "   L           : 1+3 (Command Throw - Unblockable Grab! Break with 1 or 2)\n";
    std::cout << "   P           : 3+4 (Flying Dropkick)\n";
    std::cout << " Player 2 (Tekken 7 4-Button Controls):\n";
    std::cout << "   Arrows      : Move / Guard / Crouch / Jump\n";
    std::cout << "   Num 1 or ,  : 1 (Left Punch - Flash Jab)\n";
    std::cout << "   Num 2 or .  : 2 (Right Punch - Straight Cross / Fwd+2: EWGF / 1,2: One-Two!)\n";
    std::cout << "   Num 4 or /  : 3 (Left Kick - Mid Kick / Down+3: Hell Sweep!)\n";
    std::cout << "   Num 5 or ;  : 4 (Right Kick - Axe Roundhouse / Up+4: Hopkick!)\n";
    std::cout << "   Num 3 or ]  : 1+2 (Power Crush Armor / Rage Art when in Rage!)\n";
    std::cout << "   Num 6 or '  : 1+3 (Command Throw - Unblockable Grab!)\n";
    std::cout << "   Num 9 or [  : 3+4 (Flying Dropkick)\n";
    std::cout << " Global:\n";
    std::cout << "   TAB         : Slow-Mo toggle\n";
    std::cout << "   B / Enter   : Rematch / Reset\n";
    std::cout << "====================================================\n";

    // 5. Main Game Loop
    while (window.isOpen()) {
        timeManager.update();
        float realDt = timeManager.getRealDeltaTime();

        // 1. Update Menu Manager State
        menuManager.update(realDt);
        if (menuManager.consumeQuitRequested()) {
            window.close();
        }

        if (menuManager.consumeStartMatchRequested()) {
            p1CharIdx = menuManager.getP1CharIndex();
            p2CharIdx = menuManager.getP2CharIndex();
            p1.setCharacterDef(roster[p1CharIdx]);
            p2.setCharacterDef(roster[p2CharIdx]);
            p1.respawn(sf::Vector2f(650.0f, 735.0f));
            p2.respawn(sf::Vector2f(950.0f, 735.0f));
            p1.resetRoundsWon();
            p2.resetRoundsWon();
            stageRenderer.setStage(static_cast<RagdollEngine::StageType>(menuManager.getSelectedStageIndex()));
            combatManager.startRound(1);
        }

        if (menuManager.consumeRestartMatchRequested()) {
            p1.respawn(sf::Vector2f(650.0f, 735.0f));
            p2.respawn(sf::Vector2f(950.0f, 735.0f));
            p1.resetRoundsWon();
            p2.resetRoundsWon();
            combatManager.startRound(1);
        }

        // SFML 3 Event Handling
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                sf::Vector2f mousePos(static_cast<float>(mouseMoved->position.x), static_cast<float>(mouseMoved->position.y));
                menuManager.handleMouseMove(mousePos);
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                // If not in active battle, delegate directly to menu manager
                if (menuManager.getState() != StickminGame::MenuState::Battle) {
                    menuManager.handleKeyPressed(keyPressed->code);
                } else {
                    // In-Battle Hotkeys
                    if (combatManager.getState() == StickminGame::MatchState::MatchOver) {
                        if (keyPressed->code == sf::Keyboard::Key::B || keyPressed->code == sf::Keyboard::Key::Enter) {
                            p1.respawn(sf::Vector2f(650.0f, 735.0f));
                            p2.respawn(sf::Vector2f(950.0f, 735.0f));
                            p1.resetRoundsWon();
                            p2.resetRoundsWon();
                            combatManager.startRound(1);
                            juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "REMATCH!", sf::Color(255, 230, 80), 1.6f);
                        } else if (keyPressed->code == sf::Keyboard::Key::C) {
                            menuManager.setState(StickminGame::MenuState::CharacterSelect);
                        } else if (keyPressed->code == sf::Keyboard::Key::S) {
                            menuManager.setState(StickminGame::MenuState::StageSelect);
                        } else if (keyPressed->code == sf::Keyboard::Key::M || keyPressed->code == sf::Keyboard::Key::Escape) {
                            menuManager.setState(StickminGame::MenuState::TitleScreen);
                        }
                    } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                        menuManager.setState(StickminGame::MenuState::PauseMenu);
                    } else if (keyPressed->code == sf::Keyboard::Key::F3) {
                        menuManager.handleKeyPressed(sf::Keyboard::Key::F3);
                    } else if (menuManager.getGameMode() == StickminGame::GameMode::Practice &&
                               (keyPressed->code == sf::Keyboard::Key::R)) {
                        p1.respawn(sf::Vector2f(650.0f, 735.0f));
                        p2.respawn(sf::Vector2f(950.0f, 735.0f));
                        combatManager.startRound(1);
                        juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "RESET POSITIONS!", sf::Color(80, 200, 255), 1.4f);
                    }

                    // Slow-mo toggle
                    if (keyPressed->code == sf::Keyboard::Key::Tab) {
                        if (timeManager.getTimeScale() < 0.5f) {
                            timeManager.setBaseTimeScale(1.0f);
                            timeManager.triggerSlowMo(1.0f, 0.0f);
                        } else {
                            timeManager.triggerSlowMo(0.12f, 2.0f);
                            camera.addTrauma(0.35f);
                        }
                    }

                // -------------------------------------------------------------
                // PLAYER 1 COMBAT INPUTS
                // -------------------------------------------------------------
                if (combatManager.getState() == StickminGame::MatchState::Fighting) {
                    bool p1Fwd = (p1.getController()->getFacingDirection() > 0)
                                 ? sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)
                                 : sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
                    bool p1Down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
                    bool p1Up = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);

                    // 1: LP (Flash Jab / 1+2 / 1+3)
                    if (keyPressed->code == sf::Keyboard::Key::J) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K)) {
                            // 1+2
                            if (p1.isInRage() && !p1.hasUsedRageArt()) {
                                p1.getController()->triggerMove(RagdollEngine::MoveId::RageArt);
                            } else {
                                p1.getController()->triggerMove(RagdollEngine::MoveId::PowerCrush);
                            }
                        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::U)) {
                            // 1+3
                            p1.getController()->triggerMove(RagdollEngine::MoveId::Throw);
                        } else {
                            p1Last1Time = timeManager.getGameTime();
                            p1.getController()->triggerMove(RagdollEngine::MoveId::FlashJab);
                        }
                    }
                    // 2: RP (Cross / EWGF / 1,2 String)
                    if (keyPressed->code == sf::Keyboard::Key::K) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J)) {
                            if (p1.isInRage() && !p1.hasUsedRageArt()) {
                                p1.getController()->triggerMove(RagdollEngine::MoveId::RageArt);
                            } else {
                                p1.getController()->triggerMove(RagdollEngine::MoveId::PowerCrush);
                            }
                        } else if (p1Fwd) {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::ElectricWindGodFist);
                        } else if (timeManager.getGameTime() - p1Last1Time < 0.28f) {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::OneTwoString);
                        } else {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::StraightCross);
                        }
                    }
                    // 3: LK (Mid Kick / Down+3: Hell Sweep)
                    if (keyPressed->code == sf::Keyboard::Key::U) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J)) {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::Throw);
                        } else if (p1Down) {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::HellSweep);
                        } else {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::MidKick);
                        }
                    }
                    // 4: RK (Roundhouse / Up+4: Hopkick)
                    if (keyPressed->code == sf::Keyboard::Key::I) {
                        if (p1Up) {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::Hopkick);
                        } else {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::AxeRoundhouse);
                        }
                    }
                    // O: 1+2 (Power Crush Armor / Rage Art when in Rage!)
                    if (keyPressed->code == sf::Keyboard::Key::O) {
                        if (p1.isInRage() && !p1.hasUsedRageArt()) {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::RageArt);
                        } else {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::PowerCrush);
                        }
                    }
                    // L: 1+3 (Command Throw - Unblockable Grab!)
                    if (keyPressed->code == sf::Keyboard::Key::L) {
                        p1.getController()->triggerMove(RagdollEngine::MoveId::Throw);
                    }
                    // P: 3+4 (Flying Dropkick)
                    if (keyPressed->code == sf::Keyboard::Key::P) {
                        p1.getController()->triggerMove(RagdollEngine::MoveId::FlyingDropkick);
                    }
                    // Dashing: Double-tap forward (f,f) or back (b,b) requiring key release
                    if (keyPressed->code == sf::Keyboard::Key::D) {
                        if (!p1DDown) {
                            p1DDown = true;
                            float now = timeManager.getGameTime();
                            if (now - p1LastDReleaseTime < 0.22f) {
                                p1.getController()->triggerDash(1);
                                stageRenderer.spawnGroundDust(p1.getSkeleton()->getPositionPixels() + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(-110.0f, -20.0f), 6);
                            }
                        }
                    }
                    if (keyPressed->code == sf::Keyboard::Key::A) {
                        if (!p1ADown) {
                            p1ADown = true;
                            float now = timeManager.getGameTime();
                            if (now - p1LastAReleaseTime < 0.22f) {
                                p1.getController()->triggerDash(-1);
                                stageRenderer.spawnGroundDust(p1.getSkeleton()->getPositionPixels() + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(110.0f, -20.0f), 6);
                            }
                        }
                    }

                    // Jump
                    if (keyPressed->code == sf::Keyboard::Key::W || keyPressed->code == sf::Keyboard::Key::Space) {
                        p1.getController()->jump();
                        stageRenderer.spawnGroundDust(p1.getSkeleton()->getPositionPixels() + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(0.0f, -40.0f), 8);
                    }

                    // -------------------------------------------------------------
                    // PLAYER 2 COMBAT INPUTS
                    // -------------------------------------------------------------
                    bool p2Fwd = (p2.getController()->getFacingDirection() > 0)
                                 ? sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)
                                 : sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
                    bool p2Down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
                    bool p2Up = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);

                    // 1: LP
                    if (keyPressed->code == sf::Keyboard::Key::Numpad1 || keyPressed->code == sf::Keyboard::Key::Comma) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad2) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Period)) {
                            if (p2.isInRage() && !p2.hasUsedRageArt()) {
                                p2.getController()->triggerMove(RagdollEngine::MoveId::RageArt);
                            } else {
                                p2.getController()->triggerMove(RagdollEngine::MoveId::PowerCrush);
                            }
                        } else {
                            p2Last1Time = timeManager.getGameTime();
                            p2.getController()->triggerMove(RagdollEngine::MoveId::FlashJab);
                        }
                    }
                    // 2: RP (Cross / EWGF / 1,2 String)
                    if (keyPressed->code == sf::Keyboard::Key::Numpad2 || keyPressed->code == sf::Keyboard::Key::Period) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad1) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Comma)) {
                            if (p2.isInRage() && !p2.hasUsedRageArt()) {
                                p2.getController()->triggerMove(RagdollEngine::MoveId::RageArt);
                            } else {
                                p2.getController()->triggerMove(RagdollEngine::MoveId::PowerCrush);
                            }
                        } else if (p2Fwd) {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::ElectricWindGodFist);
                        } else if (timeManager.getGameTime() - p2Last1Time < 0.28f) {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::OneTwoString);
                        } else {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::StraightCross);
                        }
                    }
                    // 3: LK (Mid Kick / Down+3: Hell Sweep)
                    if (keyPressed->code == sf::Keyboard::Key::Numpad4 || keyPressed->code == sf::Keyboard::Key::Slash) {
                        if (p2Down) {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::HellSweep);
                        } else {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::MidKick);
                        }
                    }
                    // 4: RK (Roundhouse / Up+4: Hopkick)
                    if (keyPressed->code == sf::Keyboard::Key::Numpad5 || keyPressed->code == sf::Keyboard::Key::Semicolon) {
                        if (p2Up) {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::Hopkick);
                        } else {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::AxeRoundhouse);
                        }
                    }
                    // 1+2: Power Crush / Rage Art
                    if (keyPressed->code == sf::Keyboard::Key::Numpad3 || keyPressed->code == sf::Keyboard::Key::RBracket) {
                        if (p2.isInRage() && !p2.hasUsedRageArt()) {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::RageArt);
                        } else {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::PowerCrush);
                        }
                    }
                    // 1+3: Command Throw
                    if (keyPressed->code == sf::Keyboard::Key::Numpad6 || keyPressed->code == sf::Keyboard::Key::Apostrophe) {
                        p2.getController()->triggerMove(RagdollEngine::MoveId::Throw);
                    }
                    // 3+4: Dropkick
                    if (keyPressed->code == sf::Keyboard::Key::Numpad9 || keyPressed->code == sf::Keyboard::Key::LBracket) {
                        p2.getController()->triggerMove(RagdollEngine::MoveId::FlyingDropkick);
                    }
                    // Dashing: Double-tap forward or back requiring key release
                    if (keyPressed->code == sf::Keyboard::Key::Right) {
                        if (!p2RightDown) {
                            p2RightDown = true;
                            float now = timeManager.getGameTime();
                            if (now - p2LastRightReleaseTime < 0.22f) {
                                p2.getController()->triggerDash(1);
                                stageRenderer.spawnGroundDust(p2.getSkeleton()->getPositionPixels() + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(-110.0f, -20.0f), 6);
                            }
                        }
                    }
                    if (keyPressed->code == sf::Keyboard::Key::Left) {
                        if (!p2LeftDown) {
                            p2LeftDown = true;
                            float now = timeManager.getGameTime();
                            if (now - p2LastLeftReleaseTime < 0.22f) {
                                p2.getController()->triggerDash(-1);
                                stageRenderer.spawnGroundDust(p2.getSkeleton()->getPositionPixels() + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(110.0f, -20.0f), 6);
                            }
                        }
                    }

                    // Jump
                    if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::Numpad0) {
                        p2.getController()->jump();
                        stageRenderer.spawnGroundDust(p2.getSkeleton()->getPositionPixels() + sf::Vector2f(0.0f, 65.0f), sf::Vector2f(0.0f, -40.0f), 8);
                    }
                }
            }
        }

        // Mouse Click Handling
            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
                sf::Vector2f screenPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));
                bool handled = menuManager.handleMouseClick(screenPos, mousePressed->button);

                // If match is over, check interactive button clicks
                if (!handled && menuManager.getState() == StickminGame::MenuState::Battle &&
                    combatManager.getState() == StickminGame::MatchState::MatchOver &&
                    mousePressed->button == sf::Mouse::Button::Left) {
                    if (sf::FloatRect(sf::Vector2f(260.0f, 475.0f), sf::Vector2f(220.0f, 44.0f)).contains(screenPos)) {
                        p1.respawn(sf::Vector2f(650.0f, 735.0f));
                        p2.respawn(sf::Vector2f(950.0f, 735.0f));
                        p1.resetRoundsWon();
                        p2.resetRoundsWon();
                        combatManager.startRound(1);
                        handled = true;
                    } else if (sf::FloatRect(sf::Vector2f(520.0f, 475.0f), sf::Vector2f(250.0f, 44.0f)).contains(screenPos)) {
                        menuManager.setState(StickminGame::MenuState::CharacterSelect);
                        handled = true;
                    } else if (sf::FloatRect(sf::Vector2f(810.0f, 475.0f), sf::Vector2f(220.0f, 44.0f)).contains(screenPos)) {
                        menuManager.setState(StickminGame::MenuState::StageSelect);
                        handled = true;
                    } else if (sf::FloatRect(sf::Vector2f(1070.0f, 475.0f), sf::Vector2f(220.0f, 44.0f)).contains(screenPos)) {
                        menuManager.setState(StickminGame::MenuState::TitleScreen);
                        handled = true;
                    }
                }

                // Sandbox impulse blast wave ONLY if Ctrl+Shift is held (dev cheat code)
                if (!handled && menuManager.getState() == StickminGame::MenuState::Battle) {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
                        sf::Vector2f worldPos = window.mapPixelToCoords(mousePixel, camera.getView());
                        juiceFX.spawnImpact(worldPos, sf::Vector2f(0.0f, -1.0f), sf::Color(120, 220, 255), true);
                        camera.addTrauma(0.55f);

                        b2Vec2 blastCenter = RagdollEngine::PhysicsUnits::toMeters(worldPos);
                        auto applyRadialImpulse = [&](b2BodyId body) {
                            if (b2Body_IsValid(body)) {
                                b2Vec2 bodyPos = b2Body_GetPosition(body);
                                b2Vec2 delta = { bodyPos.x - blastCenter.x, bodyPos.y - blastCenter.y };
                                float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                                if (dist < 10.0f && dist > 0.01f) {
                                    float strength = (1.0f - dist / 10.0f) * 38.0f;
                                    float inv = 1.0f / dist;
                                    b2Body_ApplyLinearImpulseToCenter(body, b2Vec2{ delta.x * inv * strength, delta.y * inv * strength - 12.0f }, true);
                                }
                            }
                        };

                        for (b2BodyId p : props) applyRadialImpulse(p);
                        for (size_t i = 0; i < static_cast<size_t>(RagdollEngine::LimbType::Count); ++i) {
                            applyRadialImpulse(p1.getSkeleton()->getBody(static_cast<RagdollEngine::LimbType>(i)));
                            applyRadialImpulse(p2.getSkeleton()->getBody(static_cast<RagdollEngine::LimbType>(i)));
                        }
                    }
                }
            }

            // Key Release Handling for Double-Tap Dash Detection
            if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
                float now = timeManager.getGameTime();
                if (keyReleased->code == sf::Keyboard::Key::D) {
                    p1DDown = false;
                    p1LastDReleaseTime = now;
                }
                if (keyReleased->code == sf::Keyboard::Key::A) {
                    p1ADown = false;
                    p1LastAReleaseTime = now;
                }
                if (keyReleased->code == sf::Keyboard::Key::Right) {
                    p2RightDown = false;
                    p2LastRightReleaseTime = now;
                }
                if (keyReleased->code == sf::Keyboard::Key::Left) {
                    p2LeftDown = false;
                    p2LastLeftReleaseTime = now;
                }
            }
        }

        // Continuous Movement Inputs
        if (menuManager.getState() == StickminGame::MenuState::Battle && combatManager.getState() == StickminGame::MatchState::Fighting) {
            float p1MoveX = 0.0f;
            float p1MoveY = 0.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) p1MoveX -= 1.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) p1MoveX += 1.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) p1MoveY += 1.0f;
            p1.getController()->setMoveInput(p1MoveX, p1MoveY);

            float p2MoveX = 0.0f;
            float p2MoveY = 0.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) p2MoveX -= 1.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) p2MoveX += 1.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) p2MoveY += 1.0f;
            p2.getController()->setMoveInput(p2MoveX, p2MoveY);
        } else {
            p1.getController()->setMoveInput(0.0f, 0.0f);
            p2.getController()->setMoveInput(0.0f, 0.0f);
        }

        if (menuManager.getState() == StickminGame::MenuState::Battle) {
            // Fixed-Timestep Physics Step
            while (timeManager.consumeFixedStep()) {
                float fixedStep = timeManager.getFixedPhysicsStep();
                p1.getController()->update(fixedStep);
                p2.getController()->update(fixedStep);
                physicsWorld.step(fixedStep);
            }

            // Update Combat & Fighters
            p1.update(realDt, juiceFX);
            p2.update(realDt, juiceFX);
            stageRenderer.update(realDt);
            combatManager.update(realDt, timeManager, camera, juiceFX, &stageRenderer);
            juiceFX.update(realDt);

            // Practice Mode auto-heal & timer hold
            if (menuManager.getGameMode() == StickminGame::GameMode::Practice) {
                combatManager.setRoundTimer(99.0f);
                if (p2.getHealth() < 40.0f) {
                    p2.resetHealth();
                }
                if (p1.getHealth() < 40.0f) {
                    p1.resetHealth();
                }
            }

            // Update Camera
            camera.clearFocusPoints();
            camera.addFocusPoint(p1.getSkeleton()->getPositionPixels());
            camera.addFocusPoint(p2.getSkeleton()->getPositionPixels());
            camera.update(realDt);
        } else if (menuManager.getState() == StickminGame::MenuState::StageSelect) {
            stageRenderer.update(realDt);
        }

        // -------------------------------------------------------------
        // RENDER PASS
        // -------------------------------------------------------------
        window.clear(sf::Color(10, 12, 16));

        bool inBattleMode = (menuManager.getState() == StickminGame::MenuState::Battle ||
                             menuManager.getState() == StickminGame::MenuState::PauseMenu ||
                             (menuManager.getState() == StickminGame::MenuState::CommandListModal && menuManager.getPrevModalState() != StickminGame::MenuState::TitleScreen));

        if (inBattleMode) {

            // World-Space Camera View
            camera.apply(window);

            // 1. Stage Parallax Background (Sky, mountains, clouds, airship hull / fortress / vault)
            stageRenderer.drawBackground(window, camera.getView());

            // 2. Stage Themed Platforms & Arena Walls
            stageRenderer.drawPlatforms(window, physicsWorld.getPlatforms());

            // 3. Props (Crates)
            for (b2BodyId propId : props) {
                if (b2Body_IsValid(propId)) {
                    sf::Vector2f pos = RagdollEngine::PhysicsUnits::toPixels(b2Body_GetPosition(propId));
                    b2Rot rot = b2Body_GetRotation(propId);
                    float rad = b2Rot_GetAngle(rot);
                    sf::RectangleShape prop(sf::Vector2f(28.0f, 28.0f));
                    prop.setOrigin(sf::Vector2f(14.0f, 14.0f));
                    prop.setPosition(pos);
                    prop.setRotation(sf::radians(rad));
                    prop.setFillColor(sf::Color(180, 140, 60));
                    prop.setOutlineColor(sf::Color(40, 30, 10));
                    prop.setOutlineThickness(2.0f);
                    window.draw(prop);
                }
            }

            // 4. Stage Wall Damage Decals (Wall splat cracks)
            stageRenderer.drawWallCracks(window);

            // 5. Ground Drop Shadows
            ragdollRenderer.drawDropShadow(window, *p1.getSkeleton(), 800.0f);
            ragdollRenderer.drawDropShadow(window, *p2.getSkeleton(), 800.0f);

            // 6. Draw Fighters
            ragdollRenderer.draw(window, *p1.getSkeleton(), p1.getController()->getFacingDirection(), p1.getTheme());
            ragdollRenderer.draw(window, *p2.getSkeleton(), p2.getController()->getFacingDirection(), p2.getTheme());

            // 7. Draw Juice FX (Motion Ribbon Trails, Starbursts, Lightning Arcs, Sparks, Shields)
            juiceFX.draw(window);

            // 8. Foreground Atmospheric Overlays & Dynamic Lighting (Snowstorm, searchlights, wind streaks, tumbleweed, dust)
            stageRenderer.drawAtmosphereAndLighting(window, camera.getView());

            // -------------------------------------------------------------
            // TEKKEN FIGHTING GAME HUD OVERLAY (Screen-Space)
            // -------------------------------------------------------------
            window.setView(window.getDefaultView());

            if (fontLoaded) {
                float topY = 32.0f;
                float barWidth = 560.0f;
                float barHeight = 26.0f;
                float ragePulse = (std::sin(timeManager.getGameTime() * 9.0f) + 1.0f) * 0.5f;

                // --- P1 HEALTH BAR (Top Left) ---
                // Background slot with Tekken 7 Rage pulse
                sf::RectangleShape p1Bg(sf::Vector2f(barWidth, barHeight));
                p1Bg.setPosition(sf::Vector2f(90.0f, topY));
                p1Bg.setFillColor(sf::Color(25, 28, 36, 230));
                if (p1.isInRage()) {
                    p1Bg.setOutlineColor(sf::Color(255, 30 + static_cast<std::uint8_t>(50 * ragePulse), 30));
                    p1Bg.setOutlineThickness(3.5f);
                } else {
                    p1Bg.setOutlineColor(sf::Color(80, 90, 110));
                    p1Bg.setOutlineThickness(2.0f);
                }
                window.draw(p1Bg);

                // Ghost Lag Bar (Yellow damage lag)
                float p1GhostWidth = (p1.getGhostHealth() / p1.getMaxHealth()) * barWidth;
                if (p1GhostWidth > 0.0f) {
                    sf::RectangleShape p1Ghost(sf::Vector2f(p1GhostWidth, barHeight));
                    p1Ghost.setPosition(sf::Vector2f(90.0f + (barWidth - p1GhostWidth), topY)); // Drains toward center
                    p1Ghost.setFillColor(sf::Color(255, 200, 50, 220));
                    window.draw(p1Ghost);
                }

                // Current Health Bar (Dynamic character accent color)
                float p1CurWidth = (p1.getHealth() / p1.getMaxHealth()) * barWidth;
                if (p1CurWidth > 0.0f) {
                    sf::RectangleShape p1Fill(sf::Vector2f(p1CurWidth, barHeight));
                    p1Fill.setPosition(sf::Vector2f(90.0f + (barWidth - p1CurWidth), topY));
                    p1Fill.setFillColor(p1.getCharacterDef().accentColor);
                    window.draw(p1Fill);
                }

                // P1 Nameplate & Title
                sf::Text p1Name(hudFont, p1.getName(), 18);
                p1Name.setStyle(sf::Text::Bold);
                p1Name.setFillColor(p1.getCharacterDef().accentColor);
                p1Name.setPosition(sf::Vector2f(90.0f, topY - 26.0f));
                window.draw(p1Name);

                // P1 RAGE Badge
                if (p1.isInRage()) {
                    sf::RectangleShape p1RageBadge(sf::Vector2f(56.0f, 18.0f));
                    p1RageBadge.setPosition(sf::Vector2f(90.0f + p1Name.getLocalBounds().size.x + 12.0f, topY - 24.0f));
                    p1RageBadge.setFillColor(sf::Color(190, 25, 25, 240));
                    p1RageBadge.setOutlineColor(sf::Color(255, 225, 40));
                    p1RageBadge.setOutlineThickness(1.5f);
                    window.draw(p1RageBadge);

                    sf::Text p1RageText(hudFont, "RAGE", 11);
                    p1RageText.setStyle(sf::Text::Bold);
                    p1RageText.setFillColor(sf::Color(255, 245, 100));
                    p1RageText.setPosition(sf::Vector2f(90.0f + p1Name.getLocalBounds().size.x + 23.0f, topY - 23.0f));
                    window.draw(p1RageText);
                }

                // P1 Victory Gems
                for (int r = 0; r < 2; ++r) {
                    sf::CircleShape gem(6.0f);
                    gem.setOrigin(sf::Vector2f(6.0f, 6.0f));
                    gem.setPosition(sf::Vector2f(620.0f - r * 20.0f, topY + barHeight + 14.0f));
                    gem.setFillColor(r < p1.getRoundsWon() ? sf::Color(255, 215, 0) : sf::Color(40, 45, 55));
                    gem.setOutlineColor(sf::Color(255, 255, 255, 160));
                    gem.setOutlineThickness(1.5f);
                    window.draw(gem);
                }

                // --- P2 HEALTH BAR (Top Right) ---
                // Background slot with Tekken 7 Rage pulse
                sf::RectangleShape p2Bg(sf::Vector2f(barWidth, barHeight));
                p2Bg.setPosition(sf::Vector2f(950.0f, topY));
                p2Bg.setFillColor(sf::Color(25, 28, 36, 230));
                if (p2.isInRage()) {
                    p2Bg.setOutlineColor(sf::Color(255, 30 + static_cast<std::uint8_t>(50 * ragePulse), 30));
                    p2Bg.setOutlineThickness(3.5f);
                } else {
                    p2Bg.setOutlineColor(sf::Color(80, 90, 110));
                    p2Bg.setOutlineThickness(2.0f);
                }
                window.draw(p2Bg);

                // Ghost Lag Bar
                float p2GhostWidth = (p2.getGhostHealth() / p2.getMaxHealth()) * barWidth;
                if (p2GhostWidth > 0.0f) {
                    sf::RectangleShape p2Ghost(sf::Vector2f(p2GhostWidth, barHeight));
                    p2Ghost.setPosition(sf::Vector2f(950.0f, topY)); // Drains toward center
                    p2Ghost.setFillColor(sf::Color(255, 200, 50, 220));
                    window.draw(p2Ghost);
                }

                // Current Health Bar (Dynamic character accent color)
                float p2CurWidth = (p2.getHealth() / p2.getMaxHealth()) * barWidth;
                if (p2CurWidth > 0.0f) {
                    sf::RectangleShape p2Fill(sf::Vector2f(p2CurWidth, barHeight));
                    p2Fill.setPosition(sf::Vector2f(950.0f, topY));
                    p2Fill.setFillColor(p2.getCharacterDef().accentColor);
                    window.draw(p2Fill);
                }

                // P2 Nameplate & Title
                sf::Text p2Name(hudFont, p2.getName(), 18);
                p2Name.setStyle(sf::Text::Bold);
                p2Name.setFillColor(p2.getCharacterDef().accentColor);
                p2Name.setOrigin(sf::Vector2f(p2Name.getLocalBounds().size.x, 0.0f));
                p2Name.setPosition(sf::Vector2f(1510.0f, topY - 26.0f));
                window.draw(p2Name);

                // P2 RAGE Badge
                if (p2.isInRage()) {
                    sf::RectangleShape p2RageBadge(sf::Vector2f(56.0f, 18.0f));
                    p2RageBadge.setPosition(sf::Vector2f(1510.0f - p2Name.getLocalBounds().size.x - 68.0f, topY - 24.0f));
                    p2RageBadge.setFillColor(sf::Color(190, 25, 25, 240));
                    p2RageBadge.setOutlineColor(sf::Color(255, 225, 40));
                    p2RageBadge.setOutlineThickness(1.5f);
                    window.draw(p2RageBadge);

                    sf::Text p2RageText(hudFont, "RAGE", 11);
                    p2RageText.setStyle(sf::Text::Bold);
                    p2RageText.setFillColor(sf::Color(255, 245, 100));
                    p2RageText.setPosition(sf::Vector2f(1510.0f - p2Name.getLocalBounds().size.x - 57.0f, topY - 23.0f));
                    window.draw(p2RageText);
                }

                // P2 Victory Gems
                for (int r = 0; r < 2; ++r) {
                    sf::CircleShape gem(6.0f);
                    gem.setOrigin(sf::Vector2f(6.0f, 6.0f));
                    gem.setPosition(sf::Vector2f(980.0f + r * 20.0f, topY + barHeight + 14.0f));
                    gem.setFillColor(r < p2.getRoundsWon() ? sf::Color(255, 215, 0) : sf::Color(40, 45, 55));
                    gem.setOutlineColor(sf::Color(255, 255, 255, 160));
                    gem.setOutlineThickness(1.5f);
                    window.draw(gem);
                }

                // --- ROUND TIMER (Top Center) ---
                sf::RectangleShape timerFrame(sf::Vector2f(110.0f, 64.0f));
                timerFrame.setOrigin(sf::Vector2f(55.0f, 32.0f));
                timerFrame.setPosition(sf::Vector2f(800.0f, topY + barHeight * 0.5f));
                timerFrame.setFillColor(sf::Color(16, 18, 24, 240));
                timerFrame.setOutlineColor(sf::Color(180, 190, 215));
                timerFrame.setOutlineThickness(2.5f);
                window.draw(timerFrame);

                int sec = static_cast<int>(std::ceil(combatManager.getRoundTimer()));
                std::string timerStr = (menuManager.getGameMode() == StickminGame::GameMode::Practice) ? "--" : std::to_string(sec);
                sf::Text timerText(hudFont, timerStr, 32);
                timerText.setStyle(sf::Text::Bold);
                timerText.setFillColor(sec <= 10 && menuManager.getGameMode() != StickminGame::GameMode::Practice ? sf::Color(255, 60, 60) : sf::Color(245, 245, 250));
                timerText.setOrigin(sf::Vector2f(timerText.getLocalBounds().size.x * 0.5f, timerText.getLocalBounds().size.y * 0.5f + 4.0f));
                timerText.setPosition(sf::Vector2f(800.0f, topY + barHeight * 0.5f));
                window.draw(timerText);

                // Round Number Banner
                std::string roundStr = (menuManager.getGameMode() == StickminGame::GameMode::Practice)
                                     ? "PRACTICE"
                                     : ("ROUND " + std::to_string(combatManager.getRoundNumber()));
                sf::Text roundSub(hudFont, roundStr, 11);
                roundSub.setStyle(sf::Text::Bold);
                roundSub.setFillColor(sf::Color(160, 175, 200));
                roundSub.setOrigin(sf::Vector2f(roundSub.getLocalBounds().size.x * 0.5f, 0.0f));
                roundSub.setPosition(sf::Vector2f(800.0f, topY + barHeight + 12.0f));
                window.draw(roundSub);

                // --- P1 DYNAMIC COMBO DISPLAY ---
                if (p1.getComboHits() > 1) {
                    sf::Text p1ComboHits(hudFont, std::to_string(p1.getComboHits()) + " HITS!", 38);
                    p1ComboHits.setStyle(sf::Text::Bold);
                    p1ComboHits.setFillColor(sf::Color(60, 160, 255));
                    p1ComboHits.setOutlineColor(sf::Color::Black);
                    p1ComboHits.setOutlineThickness(3.0f);
                    p1ComboHits.setPosition(sf::Vector2f(90.0f, 150.0f));
                    window.draw(p1ComboHits);

                    sf::Text p1ComboDmg(hudFont, "DAMAGE " + std::to_string(static_cast<int>(p1.getComboDamage())), 18);
                    p1ComboDmg.setStyle(sf::Text::Bold);
                    p1ComboDmg.setFillColor(sf::Color(255, 220, 60));
                    p1ComboDmg.setPosition(sf::Vector2f(92.0f, 200.0f));
                    window.draw(p1ComboDmg);
                }

                // --- P2 DYNAMIC COMBO DISPLAY ---
                if (p2.getComboHits() > 1) {
                    sf::Text p2ComboHits(hudFont, std::to_string(p2.getComboHits()) + " HITS!", 38);
                    p2ComboHits.setStyle(sf::Text::Bold);
                    p2ComboHits.setFillColor(sf::Color(255, 60, 80));
                    p2ComboHits.setOutlineColor(sf::Color::Black);
                    p2ComboHits.setOutlineThickness(3.0f);
                    p2ComboHits.setOrigin(sf::Vector2f(p2ComboHits.getLocalBounds().size.x, 0.0f));
                    p2ComboHits.setPosition(sf::Vector2f(1510.0f, 150.0f));
                    window.draw(p2ComboHits);

                    sf::Text p2ComboDmg(hudFont, "DAMAGE " + std::to_string(static_cast<int>(p2.getComboDamage())), 18);
                    p2ComboDmg.setStyle(sf::Text::Bold);
                    p2ComboDmg.setFillColor(sf::Color(255, 220, 60));
                    p2ComboDmg.setOrigin(sf::Vector2f(p2ComboDmg.getLocalBounds().size.x, 0.0f));
                    p2ComboDmg.setPosition(sf::Vector2f(1508.0f, 200.0f));
                    window.draw(p2ComboDmg);
                }

                // --- ROUND INTRO & MATCH OVER BANNERS ---
                if (combatManager.getState() == StickminGame::MatchState::RoundIntro) {
                    sf::Text introText(hudFont, "ROUND " + std::to_string(combatManager.getRoundNumber()) + " // FIGHT!", 54);
                    introText.setStyle(sf::Text::Bold);
                    introText.setFillColor(sf::Color(255, 235, 60));
                    introText.setOutlineColor(sf::Color(10, 10, 15));
                    introText.setOutlineThickness(4.0f);
                    introText.setOrigin(sf::Vector2f(introText.getLocalBounds().size.x * 0.5f, introText.getLocalBounds().size.y * 0.5f));
                    introText.setPosition(sf::Vector2f(800.0f, 365.0f));
                    window.draw(introText);

                    // Stage Presentation Banner
                    sf::Text stageBanner(hudFont, stageRenderer.getStageName() + "  //  " + stageRenderer.getStageSubtitle(), 15);
                    stageBanner.setStyle(sf::Text::Bold);
                    stageBanner.setFillColor(stageRenderer.getStageThemeColor());
                    stageBanner.setOutlineColor(sf::Color(10, 10, 15));
                    stageBanner.setOutlineThickness(2.5f);
                    stageBanner.setOrigin(sf::Vector2f(stageBanner.getLocalBounds().size.x * 0.5f, 0.0f));
                    stageBanner.setPosition(sf::Vector2f(800.0f, 415.0f));
                    window.draw(stageBanner);
                } else if (combatManager.getState() == StickminGame::MatchState::MatchOver) {
                    std::string winnerStr = (combatManager.getRoundWinner() == 1) ? (p1.getName() + " WINS!") : (p2.getName() + " WINS!");
                    sf::Text winText(hudFont, winnerStr, 52);
                    winText.setStyle(sf::Text::Bold);
                    winText.setFillColor(combatManager.getRoundWinner() == 1 ? p1.getCharacterDef().accentColor : p2.getCharacterDef().accentColor);
                    winText.setOutlineColor(sf::Color(10, 10, 15));
                    winText.setOutlineThickness(4.0f);
                    winText.setOrigin(sf::Vector2f(winText.getLocalBounds().size.x * 0.5f, winText.getLocalBounds().size.y * 0.5f));
                    winText.setPosition(sf::Vector2f(800.0f, 370.0f));
                    window.draw(winText);

                    // 4 Interactive Action Buttons
                    sf::Vector2i mPix = sf::Mouse::getPosition(window);
                    sf::Vector2f mouseScreenPos(static_cast<float>(mPix.x), static_cast<float>(mPix.y));

                    auto drawEndBtn = [&](float x, float y, float w, float h, const std::string& label, sf::Color col) {
                        sf::FloatRect rect(sf::Vector2f(x, y), sf::Vector2f(w, h));
                        bool hovered = rect.contains(mouseScreenPos);
                        sf::RectangleShape b(sf::Vector2f(w, h));
                        b.setPosition(sf::Vector2f(x, y));
                        b.setFillColor(hovered ? sf::Color(32, 44, 62, 245) : sf::Color(18, 22, 32, 235));
                        b.setOutlineColor(hovered ? sf::Color::White : col);
                        b.setOutlineThickness(hovered ? 3.0f : 2.0f);
                        window.draw(b);

                        sf::Text t(hudFont, label, 12);
                        t.setStyle(sf::Text::Bold);
                        t.setFillColor(hovered ? sf::Color(255, 235, 100) : sf::Color::White);
                        t.setOrigin(sf::Vector2f(t.getLocalBounds().size.x * 0.5f, t.getLocalBounds().size.y * 0.5f + 3.0f));
                        t.setPosition(sf::Vector2f(x + w * 0.5f, y + h * 0.5f));
                        window.draw(t);
                    };

                    drawEndBtn(260.0f, 475.0f, 220.0f, 44.0f, "REMATCH [ENTER]", sf::Color(255, 215, 60));
                    drawEndBtn(520.0f, 475.0f, 250.0f, 44.0f, "CHOOSE FIGHTERS [C]", sf::Color(45, 145, 255));
                    drawEndBtn(810.0f, 475.0f, 220.0f, 44.0f, "CHANGE STAGE [S]", sf::Color(245, 55, 65));
                    drawEndBtn(1070.0f, 475.0f, 220.0f, 44.0f, "MAIN MENU [ESC]", sf::Color(180, 195, 215));
                }

                // Bottom Quick Move Reference
                std::string helpStr = (menuManager.getGameMode() == StickminGame::GameMode::Practice)
                    ? "[R] Reset Fighters  |  [F3] Move List  |  [ESC] Pause Menu\n"
                      "P1: 1 (J), 2 (K), 3 (U), 4 (I) | 1,2: (J->K) | 1+2: Power Crush (O) | 1+3: Throw (L) | 3+4: Dropkick (P)\n"
                      "Fwd+2: EWGF Launcher | Down+3: Hell Sweep | Up+4: Hopkick | Block: Hold Back | Crouch Block: Down+Back"
                    : "[ESC] Pause  |  [F3] Move List  |  [TAB] Slow-Mo\n"
                      "P1: 1 (J), 2 (K), 3 (U), 4 (I) | 1,2: (J->K) | 1+2: Power Crush / Rage Art (O) | 1+3: Throw (L) | 3+4: Dropkick (P)\n"
                      "P2: 1 (Num1), 2 (Num2), 3 (Num4), 4 (Num5) | 1+2: (Num3) | 1+3: Throw (Num6) | 3+4: (Num9)\n"
                      "Fwd+2: EWGF Launcher | Down+3: Hell Sweep | Up+4: Hopkick | Block: Hold Back | Crouch Block: Down+Back";

                sf::Text moveHelp(hudFont, helpStr, 12);
                moveHelp.setFillColor(sf::Color(150, 165, 185));
                moveHelp.setOrigin(sf::Vector2f(moveHelp.getLocalBounds().size.x * 0.5f, 0.0f));
                moveHelp.setPosition(sf::Vector2f(800.0f, 835.0f));
                window.draw(moveHelp);
            }

            // If Pause Menu or Command List is active during battle, draw on top!
            if (menuManager.getState() == StickminGame::MenuState::PauseMenu ||
                menuManager.getState() == StickminGame::MenuState::CommandListModal) {
                window.setView(window.getDefaultView());
                menuManager.draw(window);
            }
        } else if (menuManager.getState() == StickminGame::MenuState::StageSelect) {
            window.setView(window.getDefaultView());
            stageRenderer.drawBackground(window, window.getDefaultView());
            stageRenderer.drawAtmosphereAndLighting(window, window.getDefaultView());
            menuManager.draw(window);
        } else {
            window.setView(window.getDefaultView());
            menuManager.draw(window);
        }

        window.display();
    }

    return 0;
}
