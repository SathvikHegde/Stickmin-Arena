#include <SFML/Graphics.hpp>
#include <RagdollEngine/Core/TimeManager.hpp>
#include <RagdollEngine/Core/CameraDirector.hpp>
#include <RagdollEngine/Physics/PhysicsWorld.hpp>
#include <RagdollEngine/Physics/PhysicsUnits.hpp>
#include <RagdollEngine/Render/RagdollRenderer.hpp>
#include <RagdollEngine/Render/JuiceFX.hpp>

#include "Fighter.hpp"
#include "CombatManager.hpp"

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
    camera.setZoomLimits(0.60f, 1.30f);

    RagdollEngine::PhysicsWorld physicsWorld(19.0f); // Balanced gravity
    RagdollEngine::RagdollRenderer ragdollRenderer;
    RagdollEngine::JuiceFX juiceFX;

    // Load HUD Font
    sf::Font hudFont;
    bool fontLoaded = hudFont.openFromFile("C:/Windows/Fonts/segoeui.ttf") ||
                      hudFont.openFromFile("C:/Windows/Fonts/arial.ttf");
    if (fontLoaded) {
        juiceFX.setFont(&hudFont);
    }

    // 3. Build Arena Geometry
    // Floor
    physicsWorld.createStaticBox(800.0f, 820.0f, 1500.0f, 40.0f, 0.95f);
    // Left & Right Arena Walls
    physicsWorld.createStaticBox(60.0f, 450.0f, 40.0f, 800.0f, 0.2f);
    physicsWorld.createStaticBox(1540.0f, 450.0f, 40.0f, 800.0f, 0.2f);
    // Floating Platforms
    physicsWorld.createStaticBox(450.0f, 620.0f, 260.0f, 20.0f, 0.85f);
    physicsWorld.createStaticBox(1150.0f, 620.0f, 260.0f, 20.0f, 0.85f);
    physicsWorld.createStaticBox(800.0f, 480.0f, 220.0f, 20.0f, 0.85f);

    // Dynamic props (crates to smash around)
    std::vector<b2BodyId> props;
    props.push_back(physicsWorld.createDynamicBox(760.0f, 420.0f, 28.0f, 28.0f, 1.2f, 0.6f));
    props.push_back(physicsWorld.createDynamicBox(840.0f, 420.0f, 28.0f, 28.0f, 1.2f, 0.6f));

    // 4. Spawn Fighters from CharacterRegistry
    const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();
    size_t p1CharIdx = 0; // Henry Stickmin
    size_t p2CharIdx = 1; // Ellie Rose

    StickminGame::Fighter p1(physicsWorld.getB2WorldId(), 1, roster[p1CharIdx], sf::Vector2f(550.0f, 735.0f));
    StickminGame::Fighter p2(physicsWorld.getB2WorldId(), 2, roster[p2CharIdx], sf::Vector2f(1050.0f, 735.0f));

    StickminGame::CombatManager combatManager(&p1, &p2);
    combatManager.startRound(1);

    std::cout << "====================================================\n";
    std::cout << " Stickmin Arena - Tekken Edition Active Ragdolls!\n";
    std::cout << " Roster: Henry, Ellie, Charles Calvin, Reginald, RHM!\n";
    std::cout << " [F1]: Cycle Player 1 Character\n";
    std::cout << " [F2]: Cycle Player 2 Character\n";
    std::cout << " Player 1:\n";
    std::cout << "   A / D       : Move / Guard (Hold Back to Block!)\n";
    std::cout << "   S           : Crouch (Hold Down+Back for Crouch Block!)\n";
    std::cout << "   W           : Jump\n";
    std::cout << "   J           : 1 (LP - Flash Jab)\n";
    std::cout << "   K           : 2 (RP - Straight Cross / Fwd+K: EWGF!)\n";
    std::cout << "   U           : 3 (LK - Low Sweep / Down+U: Hell Sweep!)\n";
    std::cout << "   I           : 4 (RK - Axe Roundhouse / Up+I: Hopkick!)\n";
    std::cout << "   O           : Flying Dropkick\n";
    std::cout << " Player 2:\n";
    std::cout << "   Arrows      : Move / Guard / Crouch / Jump\n";
    std::cout << "   Num 1 or ,  : 1 (LP - Flash Jab)\n";
    std::cout << "   Num 2 or .  : 2 (RP - Straight Cross / Fwd+2: EWGF!)\n";
    std::cout << "   Num 4 or /  : 3 (LK - Low Sweep / Down+4: Hell Sweep!)\n";
    std::cout << "   Num 5 or ;  : 4 (RK - Axe Roundhouse / Up+5: Hopkick!)\n";
    std::cout << "   Num 6 or [  : Flying Dropkick\n";
    std::cout << " Global:\n";
    std::cout << "   TAB         : Slow-Mo toggle\n";
    std::cout << "   B / Enter   : Rematch / Reset\n";
    std::cout << "====================================================\n";

    // 5. Main Game Loop
    while (window.isOpen()) {
        timeManager.update();
        float realDt = timeManager.getRealDeltaTime();

        // SFML 3 Event Handling
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }

                // Cycle Player 1 Fighter: F1 or Num7
                if (keyPressed->code == sf::Keyboard::Key::F1 || keyPressed->code == sf::Keyboard::Key::Numpad7) {
                    p1CharIdx = (p1CharIdx + 1) % roster.size();
                    p1.setCharacterDef(roster[p1CharIdx]);
                    juiceFX.spawnFloatingText(p1.getSkeleton()->getPositionPixels() - sf::Vector2f(0.0f, 65.0f),
                        p1.getName(), p1.getCharacterDef().accentColor, 1.8f);
                }

                // Cycle Player 2 Fighter: F2 or Num8
                if (keyPressed->code == sf::Keyboard::Key::F2 || keyPressed->code == sf::Keyboard::Key::Numpad8) {
                    p2CharIdx = (p2CharIdx + 1) % roster.size();
                    p2.setCharacterDef(roster[p2CharIdx]);
                    juiceFX.spawnFloatingText(p2.getSkeleton()->getPositionPixels() - sf::Vector2f(0.0f, 65.0f),
                        p2.getName(), p2.getCharacterDef().accentColor, 1.8f);
                }

                // Rematch / Reset
                if (keyPressed->code == sf::Keyboard::Key::B || keyPressed->code == sf::Keyboard::Key::Enter) {
                    p1.respawn(sf::Vector2f(550.0f, 735.0f));
                    p2.respawn(sf::Vector2f(1050.0f, 735.0f));
                    p1.resetRoundsWon();
                    p2.resetRoundsWon();
                    combatManager.startRound(1);
                    juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "REMATCH!", sf::Color(255, 230, 80), 1.6f);
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

                // Ragdoll Limp debug toggles
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    p1.getController()->toggleLimp();
                }
                if (keyPressed->code == sf::Keyboard::Key::T) {
                    p2.getController()->toggleLimp();
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

                    // 1: LP (Flash Jab)
                    if (keyPressed->code == sf::Keyboard::Key::J) {
                        p1.getController()->triggerMove(RagdollEngine::MoveId::FlashJab);
                    }
                    // 2: RP (Cross / EWGF)
                    if (keyPressed->code == sf::Keyboard::Key::K) {
                        if (p1Fwd) {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::ElectricWindGodFist);
                        } else {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::StraightCross);
                        }
                    }
                    // 3: LK (Sweep / Hell Sweep)
                    if (keyPressed->code == sf::Keyboard::Key::U) {
                        p1.getController()->triggerMove(RagdollEngine::MoveId::HellSweep);
                    }
                    // 4: RK (Roundhouse / Hopkick)
                    if (keyPressed->code == sf::Keyboard::Key::I) {
                        if (p1Up) {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::Hopkick);
                        } else {
                            p1.getController()->triggerMove(RagdollEngine::MoveId::AxeRoundhouse);
                        }
                    }
                    // Dropkick
                    if (keyPressed->code == sf::Keyboard::Key::O) {
                        p1.getController()->triggerMove(RagdollEngine::MoveId::FlyingDropkick);
                    }
                    // Jump
                    if (keyPressed->code == sf::Keyboard::Key::W || keyPressed->code == sf::Keyboard::Key::Space) {
                        p1.getController()->jump();
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
                        p2.getController()->triggerMove(RagdollEngine::MoveId::FlashJab);
                    }
                    // 2: RP (Cross / EWGF)
                    if (keyPressed->code == sf::Keyboard::Key::Numpad2 || keyPressed->code == sf::Keyboard::Key::Period) {
                        if (p2Fwd) {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::ElectricWindGodFist);
                        } else {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::StraightCross);
                        }
                    }
                    // 3: LK
                    if (keyPressed->code == sf::Keyboard::Key::Numpad4 || keyPressed->code == sf::Keyboard::Key::Slash) {
                        p2.getController()->triggerMove(RagdollEngine::MoveId::HellSweep);
                    }
                    // 4: RK (Roundhouse / Hopkick)
                    if (keyPressed->code == sf::Keyboard::Key::Numpad5 || keyPressed->code == sf::Keyboard::Key::Semicolon) {
                        if (p2Up) {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::Hopkick);
                        } else {
                            p2.getController()->triggerMove(RagdollEngine::MoveId::AxeRoundhouse);
                        }
                    }
                    // Dropkick
                    if (keyPressed->code == sf::Keyboard::Key::Numpad6 || keyPressed->code == sf::Keyboard::Key::LBracket) {
                        p2.getController()->triggerMove(RagdollEngine::MoveId::FlyingDropkick);
                    }
                    // Jump
                    if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::Numpad0) {
                        p2.getController()->jump();
                    }
                }
            }

            // Mouse Click Radial Blast Wave
            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
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

        // Continuous Movement Inputs
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
        combatManager.update(realDt, timeManager, camera, juiceFX);
        juiceFX.update(realDt);

        // Update Camera
        camera.clearFocusPoints();
        camera.addFocusPoint(p1.getSkeleton()->getPositionPixels());
        camera.addFocusPoint(p2.getSkeleton()->getPositionPixels());
        camera.update(realDt);

        // -------------------------------------------------------------
        // RENDER PASS
        // -------------------------------------------------------------
        window.clear(sf::Color(14, 16, 20));

        // World-Space Camera View
        camera.apply(window);

        // Arena Platforms
        for (const auto& platform : physicsWorld.getPlatforms()) {
            sf::RectangleShape rect(sf::Vector2f(platform.widthPixels, platform.heightPixels));
            rect.setOrigin(sf::Vector2f(platform.widthPixels * 0.5f, platform.heightPixels * 0.5f));
            rect.setPosition(platform.positionPixels);
            rect.setFillColor(sf::Color(28, 32, 40));
            rect.setOutlineColor(sf::Color(65, 75, 95));
            rect.setOutlineThickness(2.0f);
            window.draw(rect);

            sf::RectangleShape topEdge(sf::Vector2f(platform.widthPixels, 3.0f));
            topEdge.setOrigin(sf::Vector2f(platform.widthPixels * 0.5f, 1.5f));
            topEdge.setPosition(sf::Vector2f(platform.positionPixels.x, platform.positionPixels.y - platform.heightPixels * 0.5f));
            topEdge.setFillColor(sf::Color(100, 190, 255, 170));
            window.draw(topEdge);
        }

        // Props (Crates)
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

        // Ground Drop Shadows
        ragdollRenderer.drawDropShadow(window, *p1.getSkeleton(), 800.0f);
        ragdollRenderer.drawDropShadow(window, *p2.getSkeleton(), 800.0f);

        // Draw Fighters
        ragdollRenderer.draw(window, *p1.getSkeleton(), p1.getController()->getFacingDirection(), p1.getTheme());
        ragdollRenderer.draw(window, *p2.getSkeleton(), p2.getController()->getFacingDirection(), p2.getTheme());

        // Draw Juice FX (Motion Ribbon Trails, Starbursts, Lightning Arcs, Sparks, Shields)
        juiceFX.draw(window);

        // -------------------------------------------------------------
        // TEKKEN FIGHTING GAME HUD OVERLAY (Screen-Space)
        // -------------------------------------------------------------
        window.setView(window.getDefaultView());

        if (fontLoaded) {
            float topY = 32.0f;
            float barWidth = 560.0f;
            float barHeight = 26.0f;

            // --- P1 HEALTH BAR (Top Left) ---
            // Background slot
            sf::RectangleShape p1Bg(sf::Vector2f(barWidth, barHeight));
            p1Bg.setPosition(sf::Vector2f(90.0f, topY));
            p1Bg.setFillColor(sf::Color(25, 28, 36, 230));
            p1Bg.setOutlineColor(sf::Color(80, 90, 110));
            p1Bg.setOutlineThickness(2.0f);
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
            // Background slot
            sf::RectangleShape p2Bg(sf::Vector2f(barWidth, barHeight));
            p2Bg.setPosition(sf::Vector2f(950.0f, topY));
            p2Bg.setFillColor(sf::Color(25, 28, 36, 230));
            p2Bg.setOutlineColor(sf::Color(80, 90, 110));
            p2Bg.setOutlineThickness(2.0f);
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
            sf::Text timerText(hudFont, std::to_string(sec), 32);
            timerText.setStyle(sf::Text::Bold);
            timerText.setFillColor(sec <= 10 ? sf::Color(255, 60, 60) : sf::Color(245, 245, 250));
            timerText.setOrigin(sf::Vector2f(timerText.getLocalBounds().size.x * 0.5f, timerText.getLocalBounds().size.y * 0.5f + 4.0f));
            timerText.setPosition(sf::Vector2f(800.0f, topY + barHeight * 0.5f));
            window.draw(timerText);

            // Round Number Banner
            sf::Text roundSub(hudFont, "ROUND " + std::to_string(combatManager.getRoundNumber()), 11);
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
                introText.setPosition(sf::Vector2f(800.0f, 380.0f));
                window.draw(introText);
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

                sf::Text pressRText(hudFont, "PRESS [ENTER] OR [B] FOR REMATCH", 20);
                pressRText.setStyle(sf::Text::Bold);
                pressRText.setFillColor(sf::Color(255, 255, 255, 210));
                pressRText.setOrigin(sf::Vector2f(pressRText.getLocalBounds().size.x * 0.5f, 0.0f));
                pressRText.setPosition(sf::Vector2f(800.0f, 420.0f));
                window.draw(pressRText);
            }

            // Bottom Quick Move Reference
            sf::Text moveHelp(hudFont, "F1: Cycle P1 Character | F2: Cycle P2 Character | TAB: Slow-Mo | [ENTER]/[B]: Rematch\nP1: WASD + J (Jab), K (Cross / Fwd+K: EWGF), U (Sweep), I (Hopkick), O (Dropkick)\nP2: Arrows + Num 1 (Jab), Num 2 (Cross / EWGF), Num 4 (Sweep), Num 5 (Hopkick), Num 6 (Dropkick)", 12);
            moveHelp.setFillColor(sf::Color(150, 165, 185));
            moveHelp.setOrigin(sf::Vector2f(moveHelp.getLocalBounds().size.x * 0.5f, 0.0f));
            moveHelp.setPosition(sf::Vector2f(800.0f, 850.0f));
            window.draw(moveHelp);
        }

        window.display();
    }

    return 0;
}
