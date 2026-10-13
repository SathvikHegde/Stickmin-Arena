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
#include "NetworkManager.hpp"
#include "CpuController.hpp"
#include "BattleHUD.hpp"
#include "InputManager.hpp"

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

    // Resolution-independent virtual coordinate viewport (1600x900)
    sf::View uiView(sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(1600.0f, 900.0f)));
    auto updateLetterbox = [&](const sf::Vector2u& winSize) {
        float winW = static_cast<float>(winSize.x);
        float winH = static_cast<float>(winSize.y);
        if (winH <= 0.0f) winH = 1.0f;
        float winRatio = winW / winH;
        float targetRatio = 1600.0f / 900.0f; // 16:9

        float vpW = 1.0f;
        float vpH = 1.0f;
        float vpX = 0.0f;
        float vpY = 0.0f;

        if (winRatio >= targetRatio) {
            // Window is wider than 16:9 -> pillarbox (black bars on left/right)
            vpW = targetRatio / winRatio;
            vpX = (1.0f - vpW) * 0.5f;
        } else {
            // Window is taller than 16:9 -> letterbox (black bars on top/bottom)
            vpH = winRatio / targetRatio;
            vpY = (1.0f - vpH) * 0.5f;
        }

        sf::FloatRect vp(sf::Vector2f(vpX, vpY), sf::Vector2f(vpW, vpH));
        uiView.setSize(sf::Vector2f(1600.0f, 900.0f));
        uiView.setCenter(sf::Vector2f(800.0f, 450.0f));
        uiView.setViewport(vp);
        camera.setLetterboxViewport(vp);
    };
    updateLetterbox(window.getSize());

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

    StickminGame::NetworkManager netManager;
    StickminGame::MenuManager menuManager(fontLoaded ? &hudFont : nullptr, &ragdollRenderer, &stageRenderer, &netManager);
    StickminGame::BattleHUD battleHUD(fontLoaded ? &hudFont : nullptr);

    // 3. Build Arena Geometry (Tekken-style flat combat arena with left & right boundary walls)
    // Floor
    physicsWorld.createStaticBox(800.0f, 820.0f, 1500.0f, 40.0f, 0.95f);
    // Left & Right Arena Walls (Wall Splats)
    physicsWorld.createStaticBox(60.0f, 450.0f, 40.0f, 800.0f, 0.2f);
    physicsWorld.createStaticBox(1540.0f, 450.0f, 40.0f, 800.0f, 0.2f);

    // Dynamic props (empty on combat plane to maintain pure Tekken fighting flow)
    std::vector<b2BodyId> props;

    // 4. Spawn fighters from CharacterRegistry
    const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();
    size_t p1CharIdx = 0; // Henry Stickmin
    size_t p2CharIdx = 1; // Ellie Rose

    StickminGame::Fighter p1(physicsWorld.getB2WorldId(), 1, roster[p1CharIdx], sf::Vector2f(650.0f, 735.0f));
    StickminGame::Fighter p2(physicsWorld.getB2WorldId(), 2, roster[p2CharIdx], sf::Vector2f(950.0f, 735.0f));

    StickminGame::CombatManager combatManager(&p1, &p2);
    combatManager.startRound(1);

    StickminGame::CpuController cpuController;
    StickminGame::InputManager inputManager;

    float p1Last1Time = -10.0f;
    float p2Last1Time = -10.0f;

    // Multiplayer real-time state variables
    float hostClientMoveX = 0.0f;
    float hostClientMoveY = 0.0f;
    float lastClientInputTime = 0.0f;

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
        netManager.update(realDt);

        // Online Lobby & Lifecycle Synchronization
        if (menuManager.getGameMode() == StickminGame::GameMode::Online && netManager.isConnected()) {
            StickminGame::LobbySyncData peerLobby;
            if (netManager.consumeLobbySync(peerLobby)) {
                if (netManager.isClient()) {
                    menuManager.setP1CharIndex(peerLobby.p1CharIdx);
                    menuManager.setP1Ready(peerLobby.p1Ready);
                    menuManager.setSelectedStageIndex(peerLobby.stageIdx);
                    stageRenderer.setStage(static_cast<RagdollEngine::StageType>(peerLobby.stageIdx));
                    if (menuManager.getState() == StickminGame::MenuState::OnlineLobbyModal) {
                        menuManager.setState(StickminGame::MenuState::CharacterSelect);
                    }
                    if (menuManager.getState() == StickminGame::MenuState::StageSelect && (!peerLobby.p1Ready || !peerLobby.p2Ready)) {
                        menuManager.setP2Ready(false);
                        menuManager.setState(StickminGame::MenuState::CharacterSelect);
                    }
                } else if (netManager.isHost()) {
                    menuManager.setP2CharIndex(peerLobby.p2CharIdx);
                    menuManager.setP2Ready(peerLobby.p2Ready);
                    if (menuManager.getState() == StickminGame::MenuState::OnlineLobbyModal) {
                        menuManager.setState(StickminGame::MenuState::CharacterSelect);
                    }
                    if (menuManager.getState() == StickminGame::MenuState::StageSelect && (!peerLobby.p1Ready || !peerLobby.p2Ready)) {
                        menuManager.setP1Ready(false);
                        menuManager.setState(StickminGame::MenuState::CharacterSelect);
                    }
                }
            }

            if (menuManager.consumeLobbyDirty()) {
                StickminGame::LobbySyncData myLobby;
                myLobby.p1CharIdx = static_cast<uint8_t>(menuManager.getP1CharIndex());
                myLobby.p2CharIdx = static_cast<uint8_t>(menuManager.getP2CharIndex());
                myLobby.stageIdx = static_cast<uint8_t>(menuManager.getSelectedStageIndex());
                myLobby.p1Ready = menuManager.isP1Ready();
                myLobby.p2Ready = menuManager.isP2Ready();
                netManager.sendLobbySync(myLobby);
            }

            StickminGame::MatchStartData startData;
            if (netManager.isClient() && netManager.consumeMatchStart(startData)) {
                menuManager.setSelectedStageIndex(startData.stageIdx);
                menuManager.setP1CharIndex(startData.p1CharIdx);
                menuManager.setP2CharIndex(startData.p2CharIdx);
                stageRenderer.setStage(static_cast<RagdollEngine::StageType>(startData.stageIdx));
                menuManager.setState(StickminGame::MenuState::VersusIntro);
            }

            if (netManager.consumeRematch()) {
                p1.respawn(sf::Vector2f(650.0f, 735.0f));
                p2.respawn(sf::Vector2f(950.0f, 735.0f));
                p1.resetRoundsWon();
                p2.resetRoundsWon();
                inputManager.reset();
                hostClientMoveX = 0.0f;
                hostClientMoveY = 0.0f;
                p1.getController()->setMoveInput(0.0f, 0.0f);
                p2.getController()->setMoveInput(0.0f, 0.0f);
                combatManager.startRound(1);
                juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "REMATCH", sf::Color(255, 230, 80), 1.6f);
                if (netManager.isHost()) {
                    netManager.sendRematch();
                }
            }

            if (netManager.consumeReturnToLobby()) {
                menuManager.setState(StickminGame::MenuState::OnlineLobbyModal);
            }
        }

        // Handle mid-match disconnect
        if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
            if (!netManager.isConnected() && menuManager.getState() == StickminGame::MenuState::Battle) {
                juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "OPPONENT DISCONNECTED", sf::Color(255, 80, 80), 2.5f);
                menuManager.setState(StickminGame::MenuState::OnlineLobbyModal);
            }
        }

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
            inputManager.reset();
            hostClientMoveX = 0.0f;
            hostClientMoveY = 0.0f;
            p1.getController()->setMoveInput(0.0f, 0.0f);
            p2.getController()->setMoveInput(0.0f, 0.0f);
            stageRenderer.setStage(static_cast<RagdollEngine::StageType>(menuManager.getSelectedStageIndex()));
            combatManager.startRound(1);
            cpuController.reset();
            cpuController.setDifficulty(menuManager.getCpuDifficulty());
        }

        if (menuManager.consumeRestartMatchRequested()) {
            p1.respawn(sf::Vector2f(650.0f, 735.0f));
            p2.respawn(sf::Vector2f(950.0f, 735.0f));
            p1.resetRoundsWon();
            p2.resetRoundsWon();
            inputManager.reset();
            combatManager.startRound(1);
            cpuController.reset();
            cpuController.setDifficulty(menuManager.getCpuDifficulty());
        }

        // SFML 3 Event Handling
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                updateLetterbox(resized->size);
            }

            if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                sf::Vector2i pixel(mouseMoved->position.x, mouseMoved->position.y);
                sf::Vector2f uiMousePos = window.mapPixelToCoords(pixel, uiView);
                menuManager.handleMouseMove(uiMousePos);
            }

            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                menuManager.handleTextEntered(textEntered->unicode);
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                // If not in active battle, delegate directly to menu manager
                if (menuManager.getState() != StickminGame::MenuState::Battle) {
                    menuManager.handleKeyPressed(keyPressed->code);
                } else {
                    // In-Battle Hotkeys
                    if (combatManager.getState() == StickminGame::MatchState::MatchOver) {
                        if (keyPressed->code == sf::Keyboard::Key::B || keyPressed->code == sf::Keyboard::Key::Enter) {
                            if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
                                if (netManager.isHost()) {
                                    p1.respawn(sf::Vector2f(650.0f, 735.0f));
                                    p2.respawn(sf::Vector2f(950.0f, 735.0f));
                                    p1.resetRoundsWon();
                                    p2.resetRoundsWon();
                                    inputManager.reset();
                                    combatManager.startRound(1);
                                    juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "REMATCH", sf::Color(255, 230, 80), 1.6f);
                                    netManager.sendRematch();
                                } else if (netManager.isClient()) {
                                    netManager.sendRematch();
                                }
                            } else {
                                p1.respawn(sf::Vector2f(650.0f, 735.0f));
                                p2.respawn(sf::Vector2f(950.0f, 735.0f));
                                p1.resetRoundsWon();
                                p2.resetRoundsWon();
                                inputManager.reset();
                                combatManager.startRound(1);
                                juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "REMATCH", sf::Color(255, 230, 80), 1.6f);
                            }
                        } else if (keyPressed->code == sf::Keyboard::Key::C) {
                            if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
                                netManager.sendReturnToLobby();
                                menuManager.setState(StickminGame::MenuState::OnlineLobbyModal);
                            } else {
                                menuManager.setState(StickminGame::MenuState::CharacterSelect);
                            }
                        } else if (keyPressed->code == sf::Keyboard::Key::S) {
                            if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
                                netManager.sendReturnToLobby();
                                menuManager.setState(StickminGame::MenuState::OnlineLobbyModal);
                            } else {
                                menuManager.setState(StickminGame::MenuState::StageSelect);
                            }
                        } else if (keyPressed->code == sf::Keyboard::Key::M || keyPressed->code == sf::Keyboard::Key::Escape) {
                            if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
                                netManager.sendReturnToLobby();
                                netManager.disconnect();
                            }
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
                        inputManager.reset();
                        combatManager.startRound(1);
                        juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "RESET", sf::Color(80, 200, 255), 1.4f);
                    }

                    // Slow-mo toggle (offline only)
                    if (keyPressed->code == sf::Keyboard::Key::Tab && menuManager.getGameMode() != StickminGame::GameMode::Online) {
                        if (timeManager.getTimeScale() < 0.5f) {
                            timeManager.setBaseTimeScale(1.0f);
                            timeManager.triggerSlowMo(1.0f, 0.0f);
                        } else {
                            timeManager.triggerSlowMo(0.12f, 2.0f);
                            camera.addTrauma(0.35f);
                        }
                    }

                // -------------------------------------------------------------
                // COMBAT INPUTS
                // -------------------------------------------------------------
                if (combatManager.getState() == StickminGame::MatchState::Fighting) {
                    bool allowP1Arrows = (menuManager.getGameMode() == StickminGame::GameMode::Online);
                    inputManager.handleKeyPressed(keyPressed->code, timeManager.getGameTime(), allowP1Arrows);
                }
            }
        }

        // Mouse Click Handling
        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2i pixel(mousePressed->position.x, mousePressed->position.y);
            sf::Vector2f uiMousePos = window.mapPixelToCoords(pixel, uiView);
            bool handled = menuManager.handleMouseClick(uiMousePos, mousePressed->button);

            // If match is over, check interactive button clicks
            if (!handled && menuManager.getState() == StickminGame::MenuState::Battle &&
                combatManager.getState() == StickminGame::MatchState::MatchOver &&
                mousePressed->button == sf::Mouse::Button::Left) {
                auto endAction = battleHUD.checkMatchOverClick(uiMousePos);
                if (endAction == StickminGame::MatchOverAction::Rematch) {
                    if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
                        if (netManager.isHost()) {
                            p1.respawn(sf::Vector2f(650.0f, 735.0f));
                            p2.respawn(sf::Vector2f(950.0f, 735.0f));
                            p1.resetRoundsWon();
                            p2.resetRoundsWon();
                            inputManager.reset();
                            combatManager.startRound(1);
                            juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "REMATCH", sf::Color(255, 230, 80), 1.6f);
                            netManager.sendRematch();
                        } else if (netManager.isClient()) {
                            netManager.sendRematch();
                        }
                    } else {
                        p1.respawn(sf::Vector2f(650.0f, 735.0f));
                        p2.respawn(sf::Vector2f(950.0f, 735.0f));
                        p1.resetRoundsWon();
                        p2.resetRoundsWon();
                        inputManager.reset();
                        combatManager.startRound(1);
                    }
                    handled = true;
                } else if (endAction == StickminGame::MatchOverAction::ChooseFighters) {
                    if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
                        netManager.sendReturnToLobby();
                        menuManager.setState(StickminGame::MenuState::OnlineLobbyModal);
                    } else {
                        menuManager.setState(StickminGame::MenuState::CharacterSelect);
                    }
                    handled = true;
                } else if (endAction == StickminGame::MatchOverAction::ChangeStage) {
                    if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
                        netManager.sendReturnToLobby();
                        menuManager.setState(StickminGame::MenuState::OnlineLobbyModal);
                    } else {
                        menuManager.setState(StickminGame::MenuState::StageSelect);
                    }
                    handled = true;
                } else if (endAction == StickminGame::MatchOverAction::MainMenu) {
                    if (menuManager.getGameMode() == StickminGame::GameMode::Online) {
                        netManager.sendReturnToLobby();
                        netManager.disconnect();
                    }
                    menuManager.setState(StickminGame::MenuState::TitleScreen);
                    handled = true;
                }
            }

            // Sandbox impulse blast wave ONLY if Ctrl+Shift is held (dev cheat code)
            if (!handled && menuManager.getState() == StickminGame::MenuState::Battle) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
                    sf::Vector2f worldPos = window.mapPixelToCoords(pixel, camera.getView());
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
                bool allowP1Arrows = (menuManager.getGameMode() == StickminGame::GameMode::Online);
                inputManager.handleKeyReleased(keyReleased->code, timeManager.getGameTime(), allowP1Arrows);
            }
        }

        // Continuous Movement & Action Application
        if (menuManager.getState() == StickminGame::MenuState::Battle) {
            bool hasFocus = window.hasFocus();
            bool isFighting = (combatManager.getState() == StickminGame::MatchState::Fighting);

            if (menuManager.getGameMode() == StickminGame::GameMode::Online && netManager.isClient()) {
                // Online client: collect inputs and stream to host over UDP
                StickminGame::ClientInputData clientInput = inputManager.collectP1Input(hasFocus && isFighting, true);
                netManager.sendClientInput(clientInput);
            } else if (menuManager.getGameMode() == StickminGame::GameMode::Online && netManager.isHost()) {
                // Online host: P1 local inputs
                StickminGame::ClientInputData p1Input = inputManager.collectP1Input(hasFocus && isFighting, false);
                if (isFighting) {
                    StickminGame::InputManager::applyInputToFighter(p1Input, p1, timeManager.getGameTime(), p1Last1Time, &stageRenderer);
                } else {
                    p1.getController()->setMoveInput(0.0f, 0.0f);
                }

                // Host consumes incoming client inputs for P2
                StickminGame::ClientInputData cInput;
                while (netManager.consumeClientInput(cInput)) {
                    hostClientMoveX = cInput.moveX;
                    hostClientMoveY = cInput.moveY;
                    lastClientInputTime = timeManager.getGameTime();

                    if (isFighting) {
                        StickminGame::InputManager::applyInputToFighter(cInput, p2, timeManager.getGameTime(), p2Last1Time, &stageRenderer);
                    }
                }

                // Watchdog: neutralise P2 movement if client packets have timed out
                if (timeManager.getGameTime() - lastClientInputTime > 0.20f) {
                    hostClientMoveX = 0.0f;
                    hostClientMoveY = 0.0f;
                }

                if (isFighting) {
                    p2.getController()->setMoveInput(hostClientMoveX, hostClientMoveY);
                } else {
                    p2.getController()->setMoveInput(0.0f, 0.0f);
                }
            } else if (menuManager.getGameMode() == StickminGame::GameMode::VsCpu) {
                // VS CPU (P1 local, P2 driven by AI)
                StickminGame::ClientInputData p1Input = inputManager.collectP1Input(hasFocus && isFighting, false);
                if (isFighting) {
                    StickminGame::InputManager::applyInputToFighter(p1Input, p1, timeManager.getGameTime(), p1Last1Time, &stageRenderer);
                    cpuController.update(realDt, p2, p1, &stageRenderer);
                } else {
                    p1.getController()->setMoveInput(0.0f, 0.0f);
                    p2.getController()->setMoveInput(0.0f, 0.0f);
                }
            } else {
                // Offline Local 2P / Practice Mode
                StickminGame::ClientInputData p1Input = inputManager.collectP1Input(hasFocus && isFighting, false);
                StickminGame::ClientInputData p2Input = inputManager.collectP2Input(hasFocus && isFighting);
                if (isFighting) {
                    StickminGame::InputManager::applyInputToFighter(p1Input, p1, timeManager.getGameTime(), p1Last1Time, &stageRenderer);
                    StickminGame::InputManager::applyInputToFighter(p2Input, p2, timeManager.getGameTime(), p2Last1Time, &stageRenderer);
                } else {
                    p1.getController()->setMoveInput(0.0f, 0.0f);
                    p2.getController()->setMoveInput(0.0f, 0.0f);
                }
            }
        } else {
            p1.getController()->setMoveInput(0.0f, 0.0f);
            p2.getController()->setMoveInput(0.0f, 0.0f);
        }

        if (menuManager.getState() == StickminGame::MenuState::Battle) {
            bool isOnlineClient = (menuManager.getGameMode() == StickminGame::GameMode::Online && netManager.isClient());
            bool isOnlineHost = (menuManager.getGameMode() == StickminGame::GameMode::Online && netManager.isHost());

            if (isOnlineClient) {
                // Client: Drain fixed accumulator and apply received world snapshots
                while (timeManager.consumeFixedStep()) {}

                StickminGame::WorldSnapshotData snap;
                while (netManager.consumeWorldSnapshot(snap)) {
                    netManager.applySnapshot(snap, combatManager, p1, p2, juiceFX, &camera, &stageRenderer);
                }

                p1.update(realDt, juiceFX);
                p2.update(realDt, juiceFX);
                stageRenderer.update(realDt);
                juiceFX.update(realDt);
            } else {
                // Host or Offline: Authoritative simulation
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

                if (isOnlineHost) {
                    for (const auto& ev : combatManager.consumePendingJuiceEvents()) {
                        netManager.queueJuiceEvent(ev);
                    }
                    StickminGame::WorldSnapshotData snap = netManager.buildSnapshot(combatManager, p1, p2, static_cast<uint8_t>(stageRenderer.getCurrentStage()));
                    netManager.sendWorldSnapshot(snap);
                }
            }

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
            window.setView(uiView);
            battleHUD.draw(window, p1, p2, combatManager, menuManager.getGameMode(), timeManager.getGameTime(), stageRenderer, cpuController.getDifficulty(), &netManager);

            // If Pause Menu or Command List is active during battle, draw on top!
            if (menuManager.getState() == StickminGame::MenuState::PauseMenu ||
                menuManager.getState() == StickminGame::MenuState::CommandListModal) {
                window.setView(uiView);
                menuManager.draw(window);
            }
        } else if (menuManager.getState() == StickminGame::MenuState::StageSelect) {
            window.setView(uiView);
            stageRenderer.drawBackground(window, uiView);
            stageRenderer.drawAtmosphereAndLighting(window, uiView);
            menuManager.draw(window);
        } else {
            window.setView(uiView);
            menuManager.draw(window);
        }

        window.display();
    }

    return 0;
}
