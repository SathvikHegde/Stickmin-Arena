#include "BattleScene.hpp"
#include "GameContext.hpp"
#include "SceneManager.hpp"
#include "MenuManager.hpp"
#include "NetworkManager.hpp"

#include <RagdollEngine/Core/TimeManager.hpp>
#include <RagdollEngine/Physics/PhysicsUnits.hpp>
#include <RagdollEngine/Render/RagdollRenderer.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>
#include "CharacterRegistry.hpp"

#include <iostream>
#include <cmath>

namespace StickminGame {

BattleScene::BattleScene(GameContext& ctx)
    : m_ctx(ctx),
      m_battleHUD(ctx.font),
      m_camera(1600.0f, 900.0f) {
    m_camera.setZoomLimits(0.42f, 1.05f);
    m_camera.setLetterboxViewport(m_ctx.uiView.getViewport());
    if (m_ctx.font) {
        m_juiceFX.setFont(m_ctx.font);
    }
}

BattleScene::~BattleScene() = default;

void BattleScene::buildArenaGeometry() {
    if (!m_physicsWorld) return;
    // Floor
    m_physicsWorld->createStaticBox(800.0f, 820.0f, 1500.0f, 40.0f, 0.95f);
    // Left & Right Arena Walls (Wall Splats)
    m_physicsWorld->createStaticBox(60.0f, 450.0f, 40.0f, 800.0f, 0.2f);
    m_physicsWorld->createStaticBox(1540.0f, 450.0f, 40.0f, 800.0f, 0.2f);
}

void BattleScene::onEnter() {
    m_physicsWorld = std::make_unique<RagdollEngine::PhysicsWorld>(19.0f);
    buildArenaGeometry();

    const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();
    size_t p1Idx = m_ctx.menuManager.getP1CharIndex();
    size_t p2Idx = m_ctx.menuManager.getP2CharIndex();

    m_p1 = std::make_unique<Fighter>(m_physicsWorld->getB2WorldId(), 1, roster[p1Idx], sf::Vector2f(650.0f, 735.0f));
    m_p2 = std::make_unique<Fighter>(m_physicsWorld->getB2WorldId(), 2, roster[p2Idx], sf::Vector2f(950.0f, 735.0f));

    m_ctx.stageRenderer.setStage(static_cast<RagdollEngine::StageType>(m_ctx.menuManager.getSelectedStageIndex()));

    m_combatManager = std::make_unique<CombatManager>(m_p1.get(), m_p2.get());
    m_combatManager->startRound(1);

    m_cpuController.reset();
    m_cpuController.setDifficulty(m_ctx.menuManager.getCpuDifficulty());

    m_inputManager.reset();

    m_p1Last1Time = -10.0f;
    m_p2Last1Time = -10.0f;
    m_hostClientMoveX = 0.0f;
    m_hostClientMoveY = 0.0f;
    m_lastClientInputTime = 0.0f;

    m_camera.setLetterboxViewport(m_ctx.uiView.getViewport());
}

void BattleScene::onExit() {
    m_p1.reset();
    m_p2.reset();
    m_combatManager.reset();
    m_props.clear();
    m_physicsWorld.reset();
}

void BattleScene::restartMatch() {
    if (!m_p1 || !m_p2 || !m_combatManager) return;
    m_p1->respawn(sf::Vector2f(650.0f, 735.0f));
    m_p2->respawn(sf::Vector2f(950.0f, 735.0f));
    m_p1->resetRoundsWon();
    m_p2->resetRoundsWon();
    m_inputManager.reset();
    m_combatManager->startRound(1);
    m_cpuController.reset();
    m_cpuController.setDifficulty(m_ctx.menuManager.getCpuDifficulty());
}

void BattleScene::rematch() {
    if (!m_p1 || !m_p2 || !m_combatManager) return;
    m_p1->respawn(sf::Vector2f(650.0f, 735.0f));
    m_p2->respawn(sf::Vector2f(950.0f, 735.0f));
    m_p1->resetRoundsWon();
    m_p2->resetRoundsWon();
    m_inputManager.reset();
    m_hostClientMoveX = 0.0f;
    m_hostClientMoveY = 0.0f;
    m_p1->getController()->setMoveInput(0.0f, 0.0f);
    m_p2->getController()->setMoveInput(0.0f, 0.0f);
    m_combatManager->startRound(1);
    m_juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "REMATCH", sf::Color(255, 230, 80), 1.6f);
    if (m_ctx.menuManager.getGameMode() == GameMode::Online && m_ctx.netManager.isHost()) {
        m_ctx.netManager.sendRematch();
    }
}

void BattleScene::handleEvent(const sf::Event& event) {
    if (event.is<sf::Event::Resized>()) {
        m_camera.setLetterboxViewport(m_ctx.uiView.getViewport());
    }

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        if (m_ctx.menuManager.getState() == MenuState::PauseMenu ||
            m_ctx.menuManager.getState() == MenuState::CommandListModal) {
            sf::Vector2i pixel(mouseMoved->position.x, mouseMoved->position.y);
            sf::Vector2f uiMousePos = m_ctx.window.mapPixelToCoords(pixel, m_ctx.uiView);
            m_ctx.menuManager.handleMouseMove(uiMousePos);
        }
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        sf::Vector2i pixel(mousePressed->position.x, mousePressed->position.y);
        sf::Vector2f uiMousePos = m_ctx.window.mapPixelToCoords(pixel, m_ctx.uiView);

        bool handled = false;
        if (m_ctx.menuManager.getState() == MenuState::PauseMenu ||
            m_ctx.menuManager.getState() == MenuState::CommandListModal) {
            handled = m_ctx.menuManager.handleMouseClick(uiMousePos, mousePressed->button);
        }

        // If match is over, check interactive HUD button clicks
        if (!handled && m_combatManager && m_combatManager->getState() == MatchState::MatchOver &&
            mousePressed->button == sf::Mouse::Button::Left) {
            auto endAction = m_battleHUD.checkMatchOverClick(uiMousePos);
            if (endAction == MatchOverAction::Rematch) {
                if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
                    if (m_ctx.netManager.isHost()) {
                        rematch();
                    } else if (m_ctx.netManager.isClient()) {
                        m_ctx.netManager.sendRematch();
                    }
                } else {
                    rematch();
                }
                handled = true;
            } else if (endAction == MatchOverAction::ChooseFighters) {
                if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
                    m_ctx.netManager.sendReturnToLobby();
                    m_ctx.menuManager.setState(MenuState::OnlineLobbyModal);
                } else {
                    m_ctx.menuManager.setState(MenuState::CharacterSelect);
                }
                m_ctx.sceneManager.setScene(SceneType::Menu);
                handled = true;
            } else if (endAction == MatchOverAction::ChangeStage) {
                if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
                    m_ctx.netManager.sendReturnToLobby();
                    m_ctx.menuManager.setState(MenuState::OnlineLobbyModal);
                } else {
                    m_ctx.menuManager.setState(MenuState::StageSelect);
                }
                m_ctx.sceneManager.setScene(SceneType::Menu);
                handled = true;
            } else if (endAction == MatchOverAction::MainMenu) {
                if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
                    m_ctx.netManager.sendReturnToLobby();
                    m_ctx.netManager.disconnect();
                }
                m_ctx.menuManager.setState(MenuState::TitleScreen);
                m_ctx.sceneManager.setScene(SceneType::Menu);
                handled = true;
            }
        }

        // Sandbox impulse blast wave ONLY if Ctrl+Shift is held (dev cheat code)
        if (!handled && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) &&
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
            sf::Vector2f worldPos = m_ctx.window.mapPixelToCoords(pixel, m_camera.getView());
            m_juiceFX.spawnImpact(worldPos, sf::Vector2f(0.0f, -1.0f), sf::Color(120, 220, 255), true);
            m_camera.addTrauma(0.55f);

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

            for (b2BodyId p : m_props) applyRadialImpulse(p);
            if (m_p1) {
                for (size_t i = 0; i < static_cast<size_t>(RagdollEngine::LimbType::Count); ++i) {
                    applyRadialImpulse(m_p1->getSkeleton()->getBody(static_cast<RagdollEngine::LimbType>(i)));
                }
            }
            if (m_p2) {
                for (size_t i = 0; i < static_cast<size_t>(RagdollEngine::LimbType::Count); ++i) {
                    applyRadialImpulse(m_p2->getSkeleton()->getBody(static_cast<RagdollEngine::LimbType>(i)));
                }
            }
        }
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // Overlay menus handle their own keys
        if (m_ctx.menuManager.getState() == MenuState::PauseMenu ||
            m_ctx.menuManager.getState() == MenuState::CommandListModal) {
            m_ctx.menuManager.handleKeyPressed(keyPressed->code);
            return;
        }

        // In-Battle Hotkeys
        if (m_combatManager && m_combatManager->getState() == MatchState::MatchOver) {
            if (keyPressed->code == sf::Keyboard::Key::B || keyPressed->code == sf::Keyboard::Key::Enter) {
                if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
                    if (m_ctx.netManager.isHost()) {
                        rematch();
                    } else if (m_ctx.netManager.isClient()) {
                        m_ctx.netManager.sendRematch();
                    }
                } else {
                    rematch();
                }
            } else if (keyPressed->code == sf::Keyboard::Key::C) {
                if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
                    m_ctx.netManager.sendReturnToLobby();
                    m_ctx.menuManager.setState(MenuState::OnlineLobbyModal);
                } else {
                    m_ctx.menuManager.setState(MenuState::CharacterSelect);
                }
                m_ctx.sceneManager.setScene(SceneType::Menu);
                return;
            } else if (keyPressed->code == sf::Keyboard::Key::S) {
                if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
                    m_ctx.netManager.sendReturnToLobby();
                    m_ctx.menuManager.setState(MenuState::OnlineLobbyModal);
                } else {
                    m_ctx.menuManager.setState(MenuState::StageSelect);
                }
                m_ctx.sceneManager.setScene(SceneType::Menu);
                return;
            } else if (keyPressed->code == sf::Keyboard::Key::M || keyPressed->code == sf::Keyboard::Key::Escape) {
                if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
                    m_ctx.netManager.sendReturnToLobby();
                    m_ctx.netManager.disconnect();
                }
                m_ctx.menuManager.setState(MenuState::TitleScreen);
                m_ctx.sceneManager.setScene(SceneType::Menu);
                return;
            }
        } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
            m_ctx.menuManager.setState(MenuState::PauseMenu);
            return;
        } else if (keyPressed->code == sf::Keyboard::Key::F3) {
            m_ctx.menuManager.handleKeyPressed(sf::Keyboard::Key::F3);
            return;
        } else if (m_ctx.menuManager.getGameMode() == GameMode::Practice &&
                   keyPressed->code == sf::Keyboard::Key::R) {
            restartMatch();
            m_juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "RESET", sf::Color(80, 200, 255), 1.4f);
            return;
        }

        // Slow-mo toggle (offline only)
        if (keyPressed->code == sf::Keyboard::Key::Tab && m_ctx.menuManager.getGameMode() != GameMode::Online) {
            if (m_ctx.timeManager.getTimeScale() < 0.5f) {
                m_ctx.timeManager.setBaseTimeScale(1.0f);
                m_ctx.timeManager.triggerSlowMo(1.0f, 0.0f);
            } else {
                m_ctx.timeManager.triggerSlowMo(0.12f, 2.0f);
                m_camera.addTrauma(0.35f);
            }
        }

        // Combat moves input
        if (m_combatManager && m_combatManager->getState() == MatchState::Fighting) {
            bool allowP1Arrows = (m_ctx.menuManager.getGameMode() == GameMode::Online);
            m_inputManager.handleKeyPressed(keyPressed->code, m_ctx.timeManager.getGameTime(), allowP1Arrows);
        }
    }

    if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        bool allowP1Arrows = (m_ctx.menuManager.getGameMode() == GameMode::Online);
        m_inputManager.handleKeyReleased(keyReleased->code, m_ctx.timeManager.getGameTime(), allowP1Arrows);
    }
}

void BattleScene::update(float dt) {
    m_ctx.menuManager.update(dt);
    m_ctx.netManager.update(dt);

    // Check menu requests from PauseMenu
    if (m_ctx.menuManager.consumeRestartMatchRequested()) {
        restartMatch();
    }
    if (m_ctx.menuManager.consumeQuitRequested()) {
        m_ctx.window.close();
        return;
    }

    // If player navigated to menus from PauseMenu (e.g. CharacterSelect or TitleScreen)
    if (m_ctx.menuManager.getState() == MenuState::TitleScreen ||
        m_ctx.menuManager.getState() == MenuState::CharacterSelect ||
        m_ctx.menuManager.getState() == MenuState::StageSelect ||
        m_ctx.menuManager.getState() == MenuState::OnlineLobbyModal) {
        m_ctx.sceneManager.setScene(SceneType::Menu);
        return;
    }

    // Mid-match online disconnect
    if (m_ctx.menuManager.getGameMode() == GameMode::Online) {
        if (!m_ctx.netManager.isConnected()) {
            m_juiceFX.spawnFloatingText(sf::Vector2f(800.0f, 400.0f), "OPPONENT DISCONNECTED", sf::Color(255, 80, 80), 2.5f);
            m_ctx.menuManager.setState(MenuState::OnlineLobbyModal);
            m_ctx.sceneManager.setScene(SceneType::Menu);
            return;
        }
    }

    if (m_ctx.netManager.consumeRematch()) {
        rematch();
    }

    if (m_ctx.netManager.consumeReturnToLobby()) {
        m_ctx.menuManager.setState(MenuState::OnlineLobbyModal);
        m_ctx.sceneManager.setScene(SceneType::Menu);
        return;
    }

    if (!m_p1 || !m_p2 || !m_combatManager || !m_physicsWorld) return;

    // Continuous Movement & Action Application
    bool hasFocus = m_ctx.window.hasFocus();
    bool isFighting = (m_combatManager->getState() == MatchState::Fighting);

    if (m_ctx.menuManager.getGameMode() == GameMode::Online && m_ctx.netManager.isClient()) {
        // Online client: collect inputs and stream to host over UDP
        ClientInputData clientInput = m_inputManager.collectP1Input(hasFocus && isFighting, true);
        m_ctx.netManager.sendClientInput(clientInput);
    } else if (m_ctx.menuManager.getGameMode() == GameMode::Online && m_ctx.netManager.isHost()) {
        // Online host: P1 local inputs
        ClientInputData p1Input = m_inputManager.collectP1Input(hasFocus && isFighting, false);
        if (isFighting) {
            InputManager::applyInputToFighter(p1Input, *m_p1, m_ctx.timeManager.getGameTime(), m_p1Last1Time, &m_ctx.stageRenderer);
        } else {
            m_p1->getController()->setMoveInput(0.0f, 0.0f);
        }

        // Host consumes incoming client inputs for P2
        ClientInputData cInput;
        while (m_ctx.netManager.consumeClientInput(cInput)) {
            m_hostClientMoveX = cInput.moveX;
            m_hostClientMoveY = cInput.moveY;
            m_lastClientInputTime = m_ctx.timeManager.getGameTime();

            if (isFighting) {
                InputManager::applyInputToFighter(cInput, *m_p2, m_ctx.timeManager.getGameTime(), m_p2Last1Time, &m_ctx.stageRenderer);
            }
        }

        // Watchdog: neutralise P2 movement if client packets have timed out
        if (m_ctx.timeManager.getGameTime() - m_lastClientInputTime > 0.20f) {
            m_hostClientMoveX = 0.0f;
            m_hostClientMoveY = 0.0f;
        }

        if (isFighting) {
            m_p2->getController()->setMoveInput(m_hostClientMoveX, m_hostClientMoveY);
        } else {
            m_p2->getController()->setMoveInput(0.0f, 0.0f);
        }
    } else if (m_ctx.menuManager.getGameMode() == GameMode::VsCpu) {
        // VS CPU (P1 local, P2 driven by AI)
        ClientInputData p1Input = m_inputManager.collectP1Input(hasFocus && isFighting, false);
        if (isFighting) {
            InputManager::applyInputToFighter(p1Input, *m_p1, m_ctx.timeManager.getGameTime(), m_p1Last1Time, &m_ctx.stageRenderer);
            m_cpuController.update(dt, *m_p2, *m_p1, &m_ctx.stageRenderer);
        } else {
            m_p1->getController()->setMoveInput(0.0f, 0.0f);
            m_p2->getController()->setMoveInput(0.0f, 0.0f);
        }
    } else {
        // Offline Local 2P / Practice Mode
        ClientInputData p1Input = m_inputManager.collectP1Input(hasFocus && isFighting, false);
        ClientInputData p2Input = m_inputManager.collectP2Input(hasFocus && isFighting);
        if (isFighting) {
            InputManager::applyInputToFighter(p1Input, *m_p1, m_ctx.timeManager.getGameTime(), m_p1Last1Time, &m_ctx.stageRenderer);
            InputManager::applyInputToFighter(p2Input, *m_p2, m_ctx.timeManager.getGameTime(), m_p2Last1Time, &m_ctx.stageRenderer);
        } else {
            m_p1->getController()->setMoveInput(0.0f, 0.0f);
            m_p2->getController()->setMoveInput(0.0f, 0.0f);
        }
    }

    // Simulation Updates
    bool isOnlineClient = (m_ctx.menuManager.getGameMode() == GameMode::Online && m_ctx.netManager.isClient());
    bool isOnlineHost = (m_ctx.menuManager.getGameMode() == GameMode::Online && m_ctx.netManager.isHost());

    if (isOnlineClient) {
        // Client: Drain fixed accumulator and apply received world snapshots
        while (m_ctx.timeManager.consumeFixedStep()) {}

        WorldSnapshotData snap;
        while (m_ctx.netManager.consumeWorldSnapshot(snap)) {
            m_ctx.netManager.applySnapshot(snap, *m_combatManager, *m_p1, *m_p2, m_juiceFX, &m_camera, &m_ctx.stageRenderer);
        }

        m_p1->update(dt, m_juiceFX);
        m_p2->update(dt, m_juiceFX);
        m_ctx.stageRenderer.update(dt);
        m_juiceFX.update(dt);
    } else {
        // Host or Offline: Authoritative simulation
        while (m_ctx.timeManager.consumeFixedStep()) {
            float fixedStep = m_ctx.timeManager.getFixedPhysicsStep();
            m_p1->getController()->update(fixedStep);
            m_p2->getController()->update(fixedStep);
            m_physicsWorld->step(fixedStep);
        }

        m_p1->update(dt, m_juiceFX);
        m_p2->update(dt, m_juiceFX);
        m_ctx.stageRenderer.update(dt);
        m_combatManager->update(dt, m_ctx.timeManager, m_camera, m_juiceFX, &m_ctx.stageRenderer);
        m_juiceFX.update(dt);

        if (isOnlineHost) {
            for (const auto& ev : m_combatManager->consumePendingJuiceEvents()) {
                m_ctx.netManager.queueJuiceEvent(ev);
            }
            WorldSnapshotData snap = m_ctx.netManager.buildSnapshot(*m_combatManager, *m_p1, *m_p2, static_cast<uint8_t>(m_ctx.stageRenderer.getCurrentStage()));
            m_ctx.netManager.sendWorldSnapshot(snap);
        }
    }

    // Practice Mode auto-heal & timer hold
    if (m_ctx.menuManager.getGameMode() == GameMode::Practice) {
        m_combatManager->setRoundTimer(99.0f);
        if (m_p2->getHealth() < 40.0f) {
            m_p2->resetHealth();
        }
        if (m_p1->getHealth() < 40.0f) {
            m_p1->resetHealth();
        }
    }

    // Camera Framing
    m_camera.clearFocusPoints();
    m_camera.addFocusPoint(m_p1->getSkeleton()->getPositionPixels());
    m_camera.addFocusPoint(m_p2->getSkeleton()->getPositionPixels());
    m_camera.update(dt);
}

void BattleScene::render(sf::RenderWindow& window) {
    if (!m_p1 || !m_p2 || !m_combatManager || !m_physicsWorld) return;

    // World-Space Camera View
    m_camera.apply(window);

    // 1. Stage Parallax Background
    m_ctx.stageRenderer.drawBackground(window, m_camera.getView());

    // 2. Stage Themed Platforms & Arena Walls
    m_ctx.stageRenderer.drawPlatforms(window, m_physicsWorld->getPlatforms());

    // 3. Props
    for (b2BodyId propId : m_props) {
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

    // 4. Stage Wall Damage Decals
    m_ctx.stageRenderer.drawWallCracks(window);

    // 5. Ground Drop Shadows
    m_ctx.ragdollRenderer.drawDropShadow(window, *m_p1->getSkeleton(), 800.0f);
    m_ctx.ragdollRenderer.drawDropShadow(window, *m_p2->getSkeleton(), 800.0f);

    // 6. Draw Fighters
    m_ctx.ragdollRenderer.draw(window, *m_p1->getSkeleton(), m_p1->getController()->getFacingDirection(), m_p1->getTheme());
    m_ctx.ragdollRenderer.draw(window, *m_p2->getSkeleton(), m_p2->getController()->getFacingDirection(), m_p2->getTheme());

    // 7. Draw Juice FX
    m_juiceFX.draw(window);

    // 8. Atmosphere & Dynamic Lighting
    m_ctx.stageRenderer.drawAtmosphereAndLighting(window, m_camera.getView());

    // Screen-Space Tekken Fighting Game HUD
    window.setView(m_ctx.uiView);
    m_battleHUD.draw(window, *m_p1, *m_p2, *m_combatManager, m_ctx.menuManager.getGameMode(), m_ctx.timeManager.getGameTime(), m_ctx.stageRenderer, m_cpuController.getDifficulty(), &m_ctx.netManager);

    // If Pause Menu or Command List is active during battle, draw on top
    if (m_ctx.menuManager.getState() == MenuState::PauseMenu ||
        m_ctx.menuManager.getState() == MenuState::CommandListModal) {
        window.setView(m_ctx.uiView);
        m_ctx.menuManager.draw(window);
    }
}

} // namespace StickminGame
