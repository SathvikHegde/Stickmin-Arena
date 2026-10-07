#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Clipboard.hpp>

#include <RagdollEngine/Render/CharacterRegistry.hpp>
#include <RagdollEngine/Render/RagdollRenderer.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>

#include "NetworkManager.hpp"

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace StickminGame {

enum class MenuState {
    TitleScreen,
    OnlineLobbyModal,
    CpuComingSoonModal,
    CommandListModal,
    CharacterSelect,
    StageSelect,
    VersusIntro,
    Battle,
    PauseMenu
};

enum class GameMode {
    Versus,
    Online,
    Practice,
    CpuPlaceholder
};

class MenuManager {
public:
    MenuManager(const sf::Font* font, RagdollEngine::RagdollRenderer* ragdollRenderer, RagdollEngine::StageRenderer* stageRenderer, NetworkManager* netManager = nullptr)
        : m_font(font), m_ragdollRenderer(ragdollRenderer), m_stageRenderer(stageRenderer), m_netManager(netManager) {
    }

    // State getters & setters
    MenuState getState() const { return m_state; }
    void setState(MenuState s) {
        if (s == MenuState::CharacterSelect) {
            m_p1Ready = false;
            m_p2Ready = (m_gameMode == GameMode::Practice);
            m_lockinTimer = 0.0f;
        } else if (s == MenuState::TitleScreen) {
            m_p1Ready = false;
            m_p2Ready = false;
            m_lockinTimer = 0.0f;
        } else if (s == MenuState::VersusIntro) {
            m_versusTimer = 2.2f;
        }
        m_state = s;
    }

    MenuState getPrevModalState() const { return m_prevModalState; }

    GameMode getGameMode() const { return m_gameMode; }
    void setGameMode(GameMode m) { m_gameMode = m; }

    size_t getP1CharIndex() const { return m_p1CharIdx; }
    size_t getP2CharIndex() const { return m_p2CharIdx; }
    int getSelectedStageIndex() const { return m_stageIdx; }

    void setNetworkManager(NetworkManager* nm) { m_netManager = nm; }
    NetworkManager* getNetworkManager() const { return m_netManager; }

    void setP1CharIndex(size_t idx) { m_p1CharIdx = idx; }
    void setP2CharIndex(size_t idx) { m_p2CharIdx = idx; }
    void setP1Ready(bool r) { m_p1Ready = r; }
    void setP2Ready(bool r) { m_p2Ready = r; }
    bool isP1Ready() const { return m_p1Ready; }
    bool isP2Ready() const { return m_p2Ready; }
    void setSelectedStageIndex(int idx) { m_stageIdx = idx; }

    bool consumeLobbyDirty() {
        bool d = m_lobbyStateDirty;
        m_lobbyStateDirty = false;
        return d;
    }
    void markLobbyDirty() { m_lobbyStateDirty = true; }
    const std::string& getJoinIpInput() const { return m_joinIpInput; }
    void setJoinIpInput(const std::string& ip) { m_joinIpInput = ip; }

    bool isVersusIntroFinished() const { return m_state == MenuState::VersusIntro && m_versusTimer <= 0.0f; }

    // Flags for Main.cpp to act upon
    bool consumeStartMatchRequested() {
        bool req = m_startMatchRequested;
        m_startMatchRequested = false;
        return req;
    }

    bool consumeRestartMatchRequested() {
        bool req = m_restartMatchRequested;
        m_restartMatchRequested = false;
        return req;
    }

    bool consumeQuitRequested() {
        bool req = m_quitRequested;
        m_quitRequested = false;
        return req;
    }

    void update(float dt) {
        m_animTime += dt;

        if (m_state == MenuState::VersusIntro) {
            m_versusTimer -= dt;
            if (m_versusTimer <= 0.0f) {
                m_state = MenuState::Battle;
                m_startMatchRequested = true;
            }
        }

        // Online Lobby: auto-transition to Character Select once connected
        if (m_state == MenuState::OnlineLobbyModal && m_netManager && m_netManager->isConnected()) {
            m_lobbyConnectedTimer += dt;
            if (m_lobbyConnectedTimer >= 0.5f) {
                m_p1Ready = false;
                m_p2Ready = false;
                m_gameMode = GameMode::Online;
                m_state = MenuState::CharacterSelect;
                m_lobbyStateDirty = true;
                m_lobbyConnectedTimer = 0.0f;
            }
        } else {
            m_lobbyConnectedTimer = 0.0f;
        }

        // When both characters are locked in, small dramatic flash before stage select
        if (m_state == MenuState::CharacterSelect && m_p1Ready && m_p2Ready) {
            m_lockinTimer += dt;
            if (m_lockinTimer >= 0.55f) {
                m_state = MenuState::StageSelect;
                m_lockinTimer = 0.0f;
                m_lobbyStateDirty = true;
            }
        } else {
            m_lockinTimer = 0.0f;
        }
    }

    // -------------------------------------------------------------------------
    // INPUT HANDLING
    // -------------------------------------------------------------------------
    void handleKeyPressed(sf::Keyboard::Key key) {
        const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();

        // 1. Title Screen
        if (m_state == MenuState::TitleScreen) {
            if (key == sf::Keyboard::Key::W || key == sf::Keyboard::Key::Up) {
                m_titleSelectedIdx = (m_titleSelectedIdx + 5) % 6;
            } else if (key == sf::Keyboard::Key::S || key == sf::Keyboard::Key::Down) {
                m_titleSelectedIdx = (m_titleSelectedIdx + 1) % 6;
            } else if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space) {
                executeTitleAction(m_titleSelectedIdx);
            } else if (key == sf::Keyboard::Key::Escape) {
                m_quitRequested = true;
            }
        }
        // 1.5. Online Lobby Modal
        else if (m_state == MenuState::OnlineLobbyModal) {
            if (key == sf::Keyboard::Key::Escape) {
                m_state = MenuState::TitleScreen;
            } else if (key == sf::Keyboard::Key::Enter) {
                if (m_netManager && m_netManager->isConnected()) {
                    m_p1Ready = false;
                    m_p2Ready = false;
                    m_gameMode = GameMode::Online;
                    m_state = MenuState::CharacterSelect;
                    m_lobbyStateDirty = true;
                } else if (m_ipInputFocused) {
                    if (m_netManager) m_netManager->connectToHost(m_joinIpInput);
                }
            } else if (key == sf::Keyboard::Key::V && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                std::string clip = sf::Clipboard::getString().toAnsiString();
                if (!clip.empty()) {
                    m_joinIpInput = clip;
                }
            }
        }
        // 2. CPU Coming Soon Modal
        else if (m_state == MenuState::CpuComingSoonModal) {
            if (key == sf::Keyboard::Key::Escape || key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space) {
                m_state = MenuState::TitleScreen;
            }
        }
        // 3. Command List Modal
        else if (m_state == MenuState::CommandListModal) {
            if (key == sf::Keyboard::Key::Escape || key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::F3) {
                m_state = m_prevModalState;
            }
        }
        // 4. Character Select
        else if (m_state == MenuState::CharacterSelect) {
            if (m_gameMode == GameMode::Online) {
                bool isHost = (m_netManager && m_netManager->isHost());
                bool isClient = (m_netManager && m_netManager->isClient());

                if (isHost) {
                    if (!m_p1Ready) {
                        if (key == sf::Keyboard::Key::A || key == sf::Keyboard::Key::Left) {
                            m_p1CharIdx = (m_p1CharIdx + roster.size() - 1) % roster.size();
                            m_lobbyStateDirty = true;
                        } else if (key == sf::Keyboard::Key::D || key == sf::Keyboard::Key::Right) {
                            m_p1CharIdx = (m_p1CharIdx + 1) % roster.size();
                            m_lobbyStateDirty = true;
                        }
                    }
                    if (key == sf::Keyboard::Key::J || key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::Enter) {
                        m_p1Ready = !m_p1Ready;
                        m_lobbyStateDirty = true;
                    }
                } else if (isClient) {
                    if (!m_p2Ready) {
                        if (key == sf::Keyboard::Key::A || key == sf::Keyboard::Key::Left) {
                            m_p2CharIdx = (m_p2CharIdx + roster.size() - 1) % roster.size();
                            m_lobbyStateDirty = true;
                        } else if (key == sf::Keyboard::Key::D || key == sf::Keyboard::Key::Right) {
                            m_p2CharIdx = (m_p2CharIdx + 1) % roster.size();
                            m_lobbyStateDirty = true;
                        }
                    }
                    if (key == sf::Keyboard::Key::J || key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::Enter) {
                        m_p2Ready = !m_p2Ready;
                        m_lobbyStateDirty = true;
                    }
                }

                if (key == sf::Keyboard::Key::Escape) {
                    if (m_p1Ready || m_p2Ready) {
                        if (isHost) m_p1Ready = false;
                        if (isClient) m_p2Ready = false;
                        m_lobbyStateDirty = true;
                    } else {
                        m_state = MenuState::OnlineLobbyModal;
                    }
                }
            } else {
                // Player 1: A / D to navigate, J / Space to toggle ready
                if (!m_p1Ready) {
                    if (key == sf::Keyboard::Key::A) {
                        m_p1CharIdx = (m_p1CharIdx + roster.size() - 1) % roster.size();
                    } else if (key == sf::Keyboard::Key::D) {
                        m_p1CharIdx = (m_p1CharIdx + 1) % roster.size();
                    }
                }
                if (key == sf::Keyboard::Key::J || key == sf::Keyboard::Key::Space) {
                    m_p1Ready = !m_p1Ready;
                }

                // Player 2: Left / Right to navigate, Num1 / Enter to toggle ready
                if (!m_p2Ready) {
                    if (key == sf::Keyboard::Key::Left) {
                        m_p2CharIdx = (m_p2CharIdx + roster.size() - 1) % roster.size();
                    } else if (key == sf::Keyboard::Key::Right) {
                        m_p2CharIdx = (m_p2CharIdx + 1) % roster.size();
                    }
                }
                if (key == sf::Keyboard::Key::Numpad1 || key == sf::Keyboard::Key::Enter) {
                    m_p2Ready = !m_p2Ready;
                }

                if (key == sf::Keyboard::Key::Escape) {
                    if (m_p1Ready || m_p2Ready) {
                        m_p1Ready = false;
                        m_p2Ready = false;
                    } else {
                        m_state = MenuState::TitleScreen;
                    }
                }
            }
        }
        // 5. Stage Select
        else if (m_state == MenuState::StageSelect) {
            if (m_gameMode == GameMode::Online && m_netManager && m_netManager->isClient()) {
                // Client spectates stage selection, Host confirms
                if (key == sf::Keyboard::Key::Escape) {
                    m_p1Ready = false;
                    m_p2Ready = false;
                    m_state = MenuState::CharacterSelect;
                    m_lobbyStateDirty = true;
                }
            } else {
                if (key == sf::Keyboard::Key::A || key == sf::Keyboard::Key::Left) {
                    m_stageIdx = (m_stageIdx + 2) % 3;
                    if (m_stageRenderer) m_stageRenderer->setStage(static_cast<RagdollEngine::StageType>(m_stageIdx));
                    m_lobbyStateDirty = true;
                } else if (key == sf::Keyboard::Key::D || key == sf::Keyboard::Key::Right) {
                    m_stageIdx = (m_stageIdx + 1) % 3;
                    if (m_stageRenderer) m_stageRenderer->setStage(static_cast<RagdollEngine::StageType>(m_stageIdx));
                    m_lobbyStateDirty = true;
                } else if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::J) {
                    if (m_gameMode == GameMode::Online && m_netManager && m_netManager->isHost()) {
                        m_netManager->sendMatchStart(static_cast<uint8_t>(m_stageIdx), static_cast<uint8_t>(m_p1CharIdx), static_cast<uint8_t>(m_p2CharIdx));
                    }
                    m_state = MenuState::VersusIntro;
                    m_versusTimer = 2.2f;
                } else if (key == sf::Keyboard::Key::Escape) {
                    m_p1Ready = false;
                    m_p2Ready = false;
                    m_state = MenuState::CharacterSelect;
                    m_lobbyStateDirty = true;
                }
            }
        }
        // 6. Versus Intro
        else if (m_state == MenuState::VersusIntro) {
            // Any key skips directly into fight!
            if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::J || key == sf::Keyboard::Key::Escape) {
                m_versusTimer = 0.0f;
            }
        }
        // 7. Pause Menu
        else if (m_state == MenuState::PauseMenu) {
            if (key == sf::Keyboard::Key::W || key == sf::Keyboard::Key::Up) {
                m_pauseSelectedIdx = (m_pauseSelectedIdx + 4) % 5;
            } else if (key == sf::Keyboard::Key::S || key == sf::Keyboard::Key::Down) {
                m_pauseSelectedIdx = (m_pauseSelectedIdx + 1) % 5;
            } else if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space) {
                executePauseAction(m_pauseSelectedIdx);
            } else if (key == sf::Keyboard::Key::Escape) {
                m_state = MenuState::Battle;
            }
        }
        // 8. In-Battle Hotkeys
        else if (m_state == MenuState::Battle) {
            if (key == sf::Keyboard::Key::Escape) {
                m_pauseSelectedIdx = 0;
                m_state = MenuState::PauseMenu;
            } else if (key == sf::Keyboard::Key::F3) {
                m_prevModalState = MenuState::Battle;
                m_state = MenuState::CommandListModal;
            }
        }
    }

    void handleMouseMove(const sf::Vector2f& mousePos) {
        m_mousePos = mousePos;

        if (m_state == MenuState::TitleScreen) {
            for (int i = 0; i < 6; ++i) {
                sf::FloatRect btnRect(sf::Vector2f(580.0f, 300.0f + i * 54.0f), sf::Vector2f(440.0f, 42.0f));
                if (btnRect.contains(mousePos)) {
                    m_titleSelectedIdx = i;
                }
            }
        } else if (m_state == MenuState::PauseMenu) {
            for (int i = 0; i < 5; ++i) {
                sf::FloatRect btnRect(sf::Vector2f(600.0f, 290.0f + i * 62.0f), sf::Vector2f(400.0f, 48.0f));
                if (btnRect.contains(mousePos)) {
                    m_pauseSelectedIdx = i;
                }
            }
        } else if (m_state == MenuState::StageSelect) {
            // Hover position tracked in m_mousePos; stage is explicitly chosen on click or keypress
        }
    }

    void handleTextEntered(char32_t unicode) {
        if (m_state == MenuState::OnlineLobbyModal && m_ipInputFocused) {
            if (unicode == 8 || unicode == 127) { // Backspace
                if (!m_joinIpInput.empty()) {
                    m_joinIpInput.pop_back();
                }
            } else if ((unicode >= '0' && unicode <= '9') || unicode == '.' || (unicode >= 'a' && unicode <= 'z') || (unicode >= 'A' && unicode <= 'Z') || unicode == ':' || unicode == '-') {
                if (m_joinIpInput.size() < 40) {
                    m_joinIpInput += static_cast<char>(unicode);
                }
            }
        }
    }

    bool handleMouseClick(const sf::Vector2f& mousePos, sf::Mouse::Button button) {
        const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();

        // 1. Title Screen Click
        if (m_state == MenuState::TitleScreen && button == sf::Mouse::Button::Left) {
            for (int i = 0; i < 6; ++i) {
                sf::FloatRect btnRect(sf::Vector2f(580.0f, 300.0f + i * 54.0f), sf::Vector2f(440.0f, 42.0f));
                if (btnRect.contains(mousePos)) {
                    executeTitleAction(i);
                    return true;
                }
            }
        }
        // 1.5 Online Lobby Modal Click
        else if (m_state == MenuState::OnlineLobbyModal && button == sf::Mouse::Button::Left) {
            // Back Button
            sf::FloatRect backBtn(sf::Vector2f(250.0f, 660.0f), sf::Vector2f(220.0f, 46.0f));
            if (backBtn.contains(mousePos)) {
                m_state = MenuState::TitleScreen;
                return true;
            }

            // Host Button
            sf::FloatRect hostBtn(sf::Vector2f(274.0f, 545.0f), sf::Vector2f(470.0f, 48.0f));
            if (hostBtn.contains(mousePos)) {
                if (m_netManager) {
                    if (m_netManager->getStatus() == ConnectionStatus::Listening || m_netManager->isConnected()) {
                        m_netManager->disconnect();
                    } else {
                        m_netManager->startHost(24800, 24801);
                    }
                }
                return true;
            }

            // IP Input Box Focus
            sf::FloatRect ipBox(sf::Vector2f(854.0f, 378.0f), sf::Vector2f(470.0f, 44.0f));
            if (ipBox.contains(mousePos)) {
                m_ipInputFocused = true;
                return true;
            } else {
                m_ipInputFocused = false;
            }

            // Join / Connect Button
            sf::FloatRect joinBtn(sf::Vector2f(854.0f, 545.0f), sf::Vector2f(470.0f, 48.0f));
            if (joinBtn.contains(mousePos)) {
                if (m_netManager) {
                    if (m_netManager->isConnected() || m_netManager->getStatus() == ConnectionStatus::Connecting) {
                        m_netManager->disconnect();
                    } else {
                        m_netManager->connectToHost(m_joinIpInput, 24800, 24801);
                    }
                }
                return true;
            }

            // Proceed to Character Select (only when connected)
            if (m_netManager && m_netManager->isConnected()) {
                sf::FloatRect procBtn(sf::Vector2f(500.0f, 655.0f), sf::Vector2f(600.0f, 54.0f));
                if (procBtn.contains(mousePos)) {
                    m_p1Ready = false;
                    m_p2Ready = false;
                    m_gameMode = GameMode::Online;
                    m_state = MenuState::CharacterSelect;
                    m_lobbyStateDirty = true;
                    return true;
                }
            }
        }
        // 2. CPU Coming Soon Modal Click
        else if (m_state == MenuState::CpuComingSoonModal && button == sf::Mouse::Button::Left) {
            sf::FloatRect okBtn(sf::Vector2f(670.0f, 540.0f), sf::Vector2f(260.0f, 46.0f));
            if (okBtn.contains(mousePos) || !sf::FloatRect(sf::Vector2f(490.0f, 270.0f), sf::Vector2f(620.0f, 350.0f)).contains(mousePos)) {
                m_state = MenuState::TitleScreen;
                return true;
            }
        }
        // 3. Command List Modal Click
        else if (m_state == MenuState::CommandListModal && button == sf::Mouse::Button::Left) {
            sf::FloatRect closeBtn(sf::Vector2f(670.0f, 728.0f), sf::Vector2f(260.0f, 44.0f));
            if (closeBtn.contains(mousePos) || !sf::FloatRect(sf::Vector2f(250.0f, 100.0f), sf::Vector2f(1100.0f, 750.0f)).contains(mousePos)) {
                m_state = m_prevModalState;
                return true;
            }
        }
        // 4. Character Select Click
        else if (m_state == MenuState::CharacterSelect) {
            // Back to Title / Lobby Click
            sf::FloatRect backBtn(sf::Vector2f(40.0f, 30.0f), sf::Vector2f(130.0f, 36.0f));
            if (backBtn.contains(mousePos) && button == sf::Mouse::Button::Left) {
                m_p1Ready = false;
                m_p2Ready = false;
                if (m_gameMode == GameMode::Online && m_netManager) {
                    m_netManager->sendReturnToLobby();
                }
                m_state = (m_gameMode == GameMode::Online) ? MenuState::OnlineLobbyModal : MenuState::TitleScreen;
                return true;
            }

            if (m_gameMode == GameMode::Online) {
                bool isHost = (m_netManager && m_netManager->isHost());
                bool isClient = (m_netManager && m_netManager->isClient());

                // Host controls P1
                if (isHost) {
                    sf::FloatRect p1Btn(sf::Vector2f(60.0f, 670.0f), sf::Vector2f(330.0f, 80.0f));
                    if (p1Btn.contains(mousePos) && button == sf::Mouse::Button::Left) {
                        m_p1Ready = !m_p1Ready;
                        m_lobbyStateDirty = true;
                        return true;
                    }

                    if (!m_p1Ready) {
                        float startX = 415.0f;
                        float cardW = 145.0f;
                        float cardGap = 16.0f;
                        for (size_t i = 0; i < roster.size(); ++i) {
                            float cx = startX + i * (cardW + cardGap);
                            sf::FloatRect cardRect(sf::Vector2f(cx, 250.0f), sf::Vector2f(cardW, 200.0f));
                            if (cardRect.contains(mousePos) && button == sf::Mouse::Button::Left) {
                                m_p1CharIdx = i;
                                m_lobbyStateDirty = true;
                                return true;
                            }
                        }
                    }
                }
                // Client controls P2
                else if (isClient) {
                    sf::FloatRect p2Btn(sf::Vector2f(1210.0f, 670.0f), sf::Vector2f(330.0f, 80.0f));
                    if (p2Btn.contains(mousePos) && button == sf::Mouse::Button::Left) {
                        m_p2Ready = !m_p2Ready;
                        m_lobbyStateDirty = true;
                        return true;
                    }

                    if (!m_p2Ready) {
                        float startX = 415.0f;
                        float cardW = 145.0f;
                        float cardGap = 16.0f;
                        for (size_t i = 0; i < roster.size(); ++i) {
                            float cx = startX + i * (cardW + cardGap);
                            sf::FloatRect cardRect(sf::Vector2f(cx, 250.0f), sf::Vector2f(cardW, 200.0f));
                            if (cardRect.contains(mousePos) && button == sf::Mouse::Button::Left) {
                                m_p2CharIdx = i;
                                m_lobbyStateDirty = true;
                                return true;
                            }
                        }
                    }
                }
                return false;
            }

            // Local Versus Mode clicks
            sf::FloatRect p1Btn(sf::Vector2f(60.0f, 670.0f), sf::Vector2f(330.0f, 80.0f));
            if (p1Btn.contains(mousePos) && button == sf::Mouse::Button::Left) {
                m_p1Ready = !m_p1Ready;
                return true;
            }

            sf::FloatRect p2Btn(sf::Vector2f(1210.0f, 670.0f), sf::Vector2f(330.0f, 80.0f));
            if (p2Btn.contains(mousePos) && button == sf::Mouse::Button::Left) {
                m_p2Ready = !m_p2Ready;
                return true;
            }

            // Character Cards Click
            float startX = 415.0f;
            float cardW = 145.0f;
            float cardGap = 16.0f;
            for (size_t i = 0; i < roster.size(); ++i) {
                float cx = startX + i * (cardW + cardGap);
                sf::FloatRect cardRect(sf::Vector2f(cx, 250.0f), sf::Vector2f(cardW, 200.0f));
                if (cardRect.contains(mousePos)) {
                    if (button == sf::Mouse::Button::Right) {
                        m_p2CharIdx = i;
                    } else if (button == sf::Mouse::Button::Left) {
                        if (m_p1Ready && !m_p2Ready) {
                            m_p2CharIdx = i;
                        } else if (!m_p1Ready && m_p2Ready) {
                            m_p1CharIdx = i;
                        } else if (mousePos.x < cx + cardW * 0.5f) {
                            m_p1CharIdx = i;
                        } else {
                            m_p2CharIdx = i;
                        }
                    }
                    return true;
                }
            }
        }
        // 5. Stage Select Click
        else if (m_state == MenuState::StageSelect && button == sf::Mouse::Button::Left) {
            // Back button
            sf::FloatRect backBtn(sf::Vector2f(40.0f, 30.0f), sf::Vector2f(130.0f, 36.0f));
            if (backBtn.contains(mousePos)) {
                m_p1Ready = false;
                m_p2Ready = false;
                m_state = MenuState::CharacterSelect;
                m_lobbyStateDirty = true;
                return true;
            }

            // In online multiplayer, only Host has authority to select and confirm stages
            if (m_gameMode == GameMode::Online && m_netManager && m_netManager->isClient()) {
                return true;
            }

            for (int i = 0; i < 3; ++i) {
                float cx = 170.0f + i * 430.0f;
                float cy = 250.0f;
                float cardW = 390.0f;
                float cardH = 370.0f;
                sf::FloatRect cardRect(sf::Vector2f(cx, cy), sf::Vector2f(cardW, cardH));
                if (cardRect.contains(mousePos)) {
                    if (m_stageIdx != i) {
                        m_stageIdx = i;
                        if (m_stageRenderer) m_stageRenderer->setStage(static_cast<RagdollEngine::StageType>(m_stageIdx));
                        m_lobbyStateDirty = true;
                    } else {
                        if (m_gameMode == GameMode::Online && m_netManager && m_netManager->isHost()) {
                            m_netManager->sendMatchStart(static_cast<uint8_t>(m_stageIdx), static_cast<uint8_t>(m_p1CharIdx), static_cast<uint8_t>(m_p2CharIdx));
                        }
                        m_state = MenuState::VersusIntro;
                        m_versusTimer = 2.2f;
                    }
                    return true;
                }
            }
        }
        // 6. Versus Intro Click to Skip
        else if (m_state == MenuState::VersusIntro && button == sf::Mouse::Button::Left) {
            m_versusTimer = 0.0f;
            return true;
        }
        // 7. Pause Menu Click
        else if (m_state == MenuState::PauseMenu && button == sf::Mouse::Button::Left) {
            for (int i = 0; i < 5; ++i) {
                sf::FloatRect btnRect(sf::Vector2f(600.0f, 290.0f + i * 62.0f), sf::Vector2f(400.0f, 48.0f));
                if (btnRect.contains(mousePos)) {
                    executePauseAction(i);
                    return true;
                }
            }
        }

        return false;
    }

    // -------------------------------------------------------------------------
    // RENDER PASS
    // -------------------------------------------------------------------------
    void draw(sf::RenderWindow& window) {
        if (!m_font) return;

        switch (m_state) {
            case MenuState::TitleScreen:
                drawTitleScreen(window);
                break;
            case MenuState::OnlineLobbyModal:
                drawTitleScreen(window);
                drawOnlineLobbyModal(window);
                break;
            case MenuState::CpuComingSoonModal:
                drawTitleScreen(window);
                drawCpuComingSoonModal(window);
                break;
            case MenuState::CommandListModal:
                if (m_prevModalState == MenuState::TitleScreen) drawTitleScreen(window);
                drawCommandListModal(window);
                break;
            case MenuState::CharacterSelect:
                drawCharacterSelect(window);
                break;
            case MenuState::StageSelect:
                drawStageSelect(window);
                break;
            case MenuState::VersusIntro:
                drawVersusIntro(window);
                break;
            case MenuState::PauseMenu:
                drawPauseMenu(window);
                break;
            case MenuState::Battle:
                // Battle HUD is handled by Main.cpp
                break;
        }
    }

private:
    void executeTitleAction(int idx) {
        switch (idx) {
            case 0: // VERSUS BATTLE
                m_gameMode = GameMode::Versus;
                m_p1Ready = false;
                m_p2Ready = false;
                m_state = MenuState::CharacterSelect;
                break;
            case 1: // ONLINE / LAN MULTIPLAYER
                m_gameMode = GameMode::Online;
                m_prevModalState = MenuState::TitleScreen;
                m_state = MenuState::OnlineLobbyModal;
                break;
            case 2: // VS CPU (COMING SOON)
                m_gameMode = GameMode::CpuPlaceholder;
                m_state = MenuState::CpuComingSoonModal;
                break;
            case 3: // PRACTICE / TRAINING
                m_gameMode = GameMode::Practice;
                m_p1Ready = false;
                m_p2Ready = true; // Practice: P2 is dummy, already ready!
                m_state = MenuState::CharacterSelect;
                break;
            case 4: // COMMAND LIST
                m_prevModalState = MenuState::TitleScreen;
                m_state = MenuState::CommandListModal;
                break;
            case 5: // EXIT
                m_quitRequested = true;
                break;
        }
    }

    void executePauseAction(int idx) {
        switch (idx) {
            case 0: // RESUME
                m_state = MenuState::Battle;
                break;
            case 1: // COMMAND LIST
                m_prevModalState = MenuState::PauseMenu;
                m_state = MenuState::CommandListModal;
                break;
            case 2: // RESTART ROUND
                if (m_gameMode == GameMode::Online && m_netManager && m_netManager->isConnected()) {
                    m_netManager->sendRematch();
                }
                m_restartMatchRequested = true;
                m_state = MenuState::Battle;
                break;
            case 3: // CHARACTER SELECT
                if (m_gameMode == GameMode::Online && m_netManager && m_netManager->isConnected()) {
                    m_netManager->sendReturnToLobby();
                    m_state = MenuState::OnlineLobbyModal;
                } else {
                    m_p1Ready = false;
                    m_p2Ready = (m_gameMode == GameMode::Practice);
                    m_state = MenuState::CharacterSelect;
                }
                break;
            case 4: // MAIN MENU
                if (m_gameMode == GameMode::Online && m_netManager && m_netManager->isConnected()) {
                    m_netManager->sendReturnToLobby();
                    m_netManager->disconnect();
                }
                m_state = MenuState::TitleScreen;
                break;
        }
    }

    // -------------------------------------------------------------------------
    // 1. TITLE SCREEN
    // -------------------------------------------------------------------------
    void drawTitleScreen(sf::RenderWindow& window) {
        // Dark metallic gradient backdrop
        sf::RectangleShape bg(sf::Vector2f(1600.0f, 900.0f));
        bg.setFillColor(sf::Color(10, 12, 18));
        window.draw(bg);

        // Subtle animated background grid & scanlines
        float pulse = (std::sin(m_animTime * 3.5f) + 1.0f) * 0.5f;

        // Stage preview silhouette accent
        sf::CircleShape sunAccent(280.0f);
        sunAccent.setOrigin(sf::Vector2f(280.0f, 280.0f));
        sunAccent.setPosition(sf::Vector2f(800.0f, 220.0f));
        sunAccent.setFillColor(sf::Color(245, 55, 65, 22 + static_cast<std::uint8_t>(pulse * 15.0f)));
        window.draw(sunAccent);

        // Logo Top Title
        sf::Text titleMain(*m_font, "STICKMIN ARENA", 68);
        titleMain.setStyle(sf::Text::Bold);
        titleMain.setFillColor(sf::Color(255, 255, 255));
        titleMain.setOutlineColor(sf::Color(45, 145, 255));
        titleMain.setOutlineThickness(4.0f);
        titleMain.setOrigin(sf::Vector2f(titleMain.getLocalBounds().size.x * 0.5f, 0.0f));
        titleMain.setPosition(sf::Vector2f(800.0f, 105.0f));
        window.draw(titleMain);

        // Subtitle
        sf::Text sub(*m_font, "THE RAGDOLL IRON FIST TOURNAMENT", 16);
        sub.setStyle(sf::Text::Bold);
        sub.setFillColor(sf::Color(255, 215, 60));
        sub.setOrigin(sf::Vector2f(sub.getLocalBounds().size.x * 0.5f, 0.0f));
        sub.setPosition(sf::Vector2f(800.0f, 195.0f));
        window.draw(sub);

        // Version badge
        sf::Text ver(*m_font, "Version: 0.67", 12);
        ver.setFillColor(sf::Color(130, 145, 170));
        ver.setOrigin(sf::Vector2f(ver.getLocalBounds().size.x * 0.5f, 0.0f));
        ver.setPosition(sf::Vector2f(800.0f, 224.0f));
        window.draw(ver);

        // Menu Options
        const std::vector<std::string> options = {
            "VERSUS BATTLE (LOCAL 2P)",
            "ONLINE / LAN MULTIPLAYER",
            "VS CPU (SINGLE PLAYER)",
            "PRACTICE / TRAINING",
            "COMMAND LIST & MOVES",
            "EXIT GAME"
        };

        for (size_t i = 0; i < options.size(); ++i) {
            bool isSelected = (static_cast<int>(i) == m_titleSelectedIdx);
            float btnY = 300.0f + i * 54.0f;

            sf::RectangleShape btn(sf::Vector2f(440.0f, 42.0f));
            btn.setOrigin(sf::Vector2f(220.0f, 21.0f));
            btn.setPosition(sf::Vector2f(800.0f, btnY + 21.0f));

            if (isSelected) {
                btn.setFillColor(sf::Color(30, 42, 65, 240));
                btn.setOutlineColor(sf::Color(255, 215, 60));
                btn.setOutlineThickness(2.8f);
            } else {
                btn.setFillColor(sf::Color(18, 22, 30, 200));
                btn.setOutlineColor(sf::Color(55, 65, 85));
                btn.setOutlineThickness(1.5f);
            }
            window.draw(btn);

            // Option Text
            sf::Text optText(*m_font, options[i], 16);
            optText.setStyle(sf::Text::Bold);
            optText.setFillColor(isSelected ? sf::Color(255, 255, 255) : sf::Color(190, 205, 225));
            optText.setOrigin(sf::Vector2f(optText.getLocalBounds().size.x * 0.5f, optText.getLocalBounds().size.y * 0.5f + 3.0f));
            optText.setPosition(sf::Vector2f(800.0f, btnY + 21.0f));
            window.draw(optText);

            // "NEW / NETPLAY" pill on Online option (index 1)
            if (i == 1) {
                sf::RectangleShape netPill(sf::Vector2f(95.0f, 18.0f));
                netPill.setOrigin(sf::Vector2f(0.0f, 9.0f));
                netPill.setPosition(sf::Vector2f(1030.0f, btnY + 21.0f));
                netPill.setFillColor(sf::Color(30, 160, 240));
                netPill.setOutlineColor(sf::Color(255, 215, 60));
                netPill.setOutlineThickness(1.2f);
                window.draw(netPill);

                sf::Text netText(*m_font, "NEW / NETPLAY", 9);
                netText.setStyle(sf::Text::Bold);
                netText.setFillColor(sf::Color::White);
                netText.setPosition(sf::Vector2f(1034.0f, btnY + 15.0f));
                window.draw(netText);
            }
            // "COMING SOON" small pill on CPU option (index 2)
            else if (i == 2) {
                sf::RectangleShape csPill(sf::Vector2f(95.0f, 18.0f));
                csPill.setOrigin(sf::Vector2f(0.0f, 9.0f));
                csPill.setPosition(sf::Vector2f(1030.0f, btnY + 21.0f));
                csPill.setFillColor(sf::Color(220, 50, 50));
                csPill.setOutlineColor(sf::Color(255, 215, 60));
                csPill.setOutlineThickness(1.2f);
                window.draw(csPill);

                sf::Text csText(*m_font, "COMING SOON", 9);
                csText.setStyle(sf::Text::Bold);
                csText.setFillColor(sf::Color::White);
                csText.setPosition(sf::Vector2f(1036.0f, btnY + 15.0f));
                window.draw(csText);
            }

            // Arrow on selected
            if (isSelected) {
                sf::Text arrow(*m_font, ">", 20);
                arrow.setStyle(sf::Text::Bold);
                arrow.setFillColor(sf::Color(255, 215, 60));
                arrow.setPosition(sf::Vector2f(595.0f, btnY + 8.0f));
                window.draw(arrow);
            }
        }

        // Bottom Controls Hint
        sf::Text hint(*m_font, "[W / S] or [UP / DOWN] Navigate   |   [ENTER] or [SPACE] Select   |   [ESC] Quit   |   MOUSE SUPPORTED", 13);
        hint.setFillColor(sf::Color(140, 155, 180));
        hint.setOrigin(sf::Vector2f(hint.getLocalBounds().size.x * 0.5f, 0.0f));
        hint.setPosition(sf::Vector2f(800.0f, 840.0f));
        window.draw(hint);
    }

    // -------------------------------------------------------------------------
    // 1.5 ONLINE LOBBY MODAL
    // -------------------------------------------------------------------------
    void drawOnlineLobbyModal(sf::RenderWindow& window) {
        // Dark translucent overlay
        sf::RectangleShape mask(sf::Vector2f(1600.0f, 900.0f));
        mask.setFillColor(sf::Color(8, 10, 16, 235));
        window.draw(mask);

        // Main modal container (1200 x 740)
        sf::RectangleShape box(sf::Vector2f(1200.0f, 740.0f));
        box.setOrigin(sf::Vector2f(600.0f, 370.0f));
        box.setPosition(sf::Vector2f(800.0f, 450.0f));
        box.setFillColor(sf::Color(14, 18, 28, 250));
        box.setOutlineColor(sf::Color(45, 145, 255));
        box.setOutlineThickness(3.0f);
        window.draw(box);

        // Header Title
        sf::Text title(*m_font, "ONLINE & LAN MULTIPLAYER LOBBY", 26);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color::White);
        title.setOutlineColor(sf::Color(45, 145, 255));
        title.setOutlineThickness(2.5f);
        title.setOrigin(sf::Vector2f(title.getLocalBounds().size.x * 0.5f, 0.0f));
        title.setPosition(sf::Vector2f(800.0f, 105.0f));
        window.draw(title);

        sf::Text sub(*m_font, "DIRECT HIGH-SPEED UDP / TCP PEER-TO-PEER // ZERO CLOUD REQUIREMENT", 12);
        sub.setFillColor(sf::Color(255, 215, 60));
        sub.setOrigin(sf::Vector2f(sub.getLocalBounds().size.x * 0.5f, 0.0f));
        sub.setPosition(sf::Vector2f(800.0f, 145.0f));
        window.draw(sub);

        // Status Banner Bar
        float bannerY = 175.0f;
        sf::RectangleShape statusBg(sf::Vector2f(1100.0f, 38.0f));
        statusBg.setOrigin(sf::Vector2f(550.0f, 19.0f));
        statusBg.setPosition(sf::Vector2f(800.0f, bannerY + 19.0f));

        ConnectionStatus status = m_netManager ? m_netManager->getStatus() : ConnectionStatus::Disconnected;
        sf::Color statusOutlineCol(55, 70, 95);
        sf::Color statusTextCol(180, 200, 225);
        std::string statusStr = m_netManager ? m_netManager->getStatusMessage() : "Disconnected";

        if (status == ConnectionStatus::Listening) {
            statusOutlineCol = sf::Color(255, 200, 40);
            statusTextCol = sf::Color(255, 225, 100);
            statusBg.setFillColor(sf::Color(40, 32, 16));
        } else if (status == ConnectionStatus::Connecting) {
            statusOutlineCol = sf::Color(45, 165, 255);
            statusTextCol = sf::Color(140, 215, 255);
            statusBg.setFillColor(sf::Color(18, 30, 48));
        } else if (status == ConnectionStatus::Connected) {
            statusOutlineCol = sf::Color(50, 230, 120);
            statusTextCol = sf::Color(120, 255, 170);
            statusBg.setFillColor(sf::Color(16, 40, 24));
            statusStr += " | PING: " + std::to_string(static_cast<int>(m_netManager->getPingMs())) + " ms";
        } else {
            statusBg.setFillColor(sf::Color(22, 28, 40));
        }

        statusBg.setOutlineColor(statusOutlineCol);
        statusBg.setOutlineThickness(1.8f);
        window.draw(statusBg);

        sf::Text statusText(*m_font, statusStr, 13);
        statusText.setStyle(sf::Text::Bold);
        statusText.setFillColor(statusTextCol);
        statusText.setOrigin(sf::Vector2f(statusText.getLocalBounds().size.x * 0.5f, statusText.getLocalBounds().size.y * 0.5f + 2.0f));
        statusText.setPosition(sf::Vector2f(800.0f, bannerY + 19.0f));
        window.draw(statusText);

        // ---------------------------------------------------------------------
        // LEFT CARD: HOST MATCH (Player 1)
        // ---------------------------------------------------------------------
        float cardY = 240.0f;
        float cardW = 520.0f;
        float cardH = 390.0f;
        float leftX = 250.0f;
        float rightX = 830.0f;

        sf::RectangleShape hostCard(sf::Vector2f(cardW, cardH));
        hostCard.setPosition(sf::Vector2f(leftX, cardY));
        hostCard.setFillColor(sf::Color(20, 26, 38));
        hostCard.setOutlineColor(sf::Color(45, 145, 255));
        hostCard.setOutlineThickness(2.0f);
        window.draw(hostCard);

        sf::Text hostTitle(*m_font, "HOST MATCH (PLAYER 1)", 18);
        hostTitle.setStyle(sf::Text::Bold);
        hostTitle.setFillColor(sf::Color(80, 180, 255));
        hostTitle.setPosition(sf::Vector2f(leftX + 24.0f, cardY + 20.0f));
        window.draw(hostTitle);

        sf::Text hostDesc(*m_font, 
            "Runs the authoritative Box2D physics simulation.\n"
            "Share your Local or Public IP with Player 2 to join.", 12);
        hostDesc.setFillColor(sf::Color(190, 205, 225));
        hostDesc.setPosition(sf::Vector2f(leftX + 24.0f, cardY + 54.0f));
        window.draw(hostDesc);

        // Local IP Display Area
        sf::Text ipLabel(*m_font, "YOUR IP ADDRESS (SHARE WITH OPPONENT):", 11);
        ipLabel.setStyle(sf::Text::Bold);
        ipLabel.setFillColor(sf::Color(255, 215, 60));
        ipLabel.setPosition(sf::Vector2f(leftX + 24.0f, cardY + 115.0f));
        window.draw(ipLabel);

        sf::RectangleShape ipDisplayBg(sf::Vector2f(470.0f, 44.0f));
        ipDisplayBg.setPosition(sf::Vector2f(leftX + 24.0f, cardY + 138.0f));
        ipDisplayBg.setFillColor(sf::Color(12, 16, 24));
        ipDisplayBg.setOutlineColor(sf::Color(55, 75, 105));
        ipDisplayBg.setOutlineThickness(1.5f);
        window.draw(ipDisplayBg);

        std::string myIp = m_netManager ? m_netManager->getLocalIpString() : "127.0.0.1";
        sf::Text myIpText(*m_font, myIp + "  (Port 24800)", 17);
        myIpText.setStyle(sf::Text::Bold);
        myIpText.setFillColor(sf::Color(255, 255, 255));
        myIpText.setPosition(sf::Vector2f(leftX + 38.0f, cardY + 148.0f));
        window.draw(myIpText);

        sf::Text hostInfo(*m_font, 
            "Network Protocol: TCP 24800 / UDP 24801\n"
            "LAN: Works out of the box!\n"
            "Internet: Requires Port Forwarding / Radmin / Tailscale", 11);
        hostInfo.setFillColor(sf::Color(140, 160, 185));
        hostInfo.setPosition(sf::Vector2f(leftX + 24.0f, cardY + 205.0f));
        window.draw(hostInfo);

        // Host Button
        bool isHostActive = (m_netManager && (m_netManager->getStatus() == ConnectionStatus::Listening || (m_netManager->isHost() && m_netManager->isConnected())));
        sf::FloatRect hostBtnRect(sf::Vector2f(leftX + 24.0f, cardY + 305.0f), sf::Vector2f(470.0f, 48.0f));
        bool hostHover = hostBtnRect.contains(m_mousePos);

        sf::RectangleShape hostBtn(sf::Vector2f(470.0f, 48.0f));
        hostBtn.setPosition(sf::Vector2f(leftX + 24.0f, cardY + 305.0f));
        if (isHostActive) {
            hostBtn.setFillColor(hostHover ? sf::Color(230, 60, 60) : sf::Color(190, 40, 40));
            hostBtn.setOutlineColor(sf::Color(255, 120, 120));
        } else {
            hostBtn.setFillColor(hostHover ? sf::Color(45, 145, 255) : sf::Color(28, 105, 210));
            hostBtn.setOutlineColor(hostHover ? sf::Color(255, 215, 60) : sf::Color(80, 180, 255));
        }
        hostBtn.setOutlineThickness(hostHover ? 2.5f : 1.5f);
        window.draw(hostBtn);

        sf::Text hostBtnText(*m_font, isHostActive ? "STOP HOSTING" : "START HOSTING MATCH", 15);
        hostBtnText.setStyle(sf::Text::Bold);
        hostBtnText.setFillColor(sf::Color::White);
        hostBtnText.setOrigin(sf::Vector2f(hostBtnText.getLocalBounds().size.x * 0.5f, hostBtnText.getLocalBounds().size.y * 0.5f + 2.0f));
        hostBtnText.setPosition(sf::Vector2f(leftX + 24.0f + 235.0f, cardY + 329.0f));
        window.draw(hostBtnText);

        // ---------------------------------------------------------------------
        // RIGHT CARD: JOIN MATCH (Player 2)
        // ---------------------------------------------------------------------
        sf::RectangleShape joinCard(sf::Vector2f(cardW, cardH));
        joinCard.setPosition(sf::Vector2f(rightX, cardY));
        joinCard.setFillColor(sf::Color(20, 26, 38));
        joinCard.setOutlineColor(sf::Color(245, 55, 65));
        joinCard.setOutlineThickness(2.0f);
        window.draw(joinCard);

        sf::Text joinTitle(*m_font, "JOIN MATCH (PLAYER 2)", 18);
        joinTitle.setStyle(sf::Text::Bold);
        joinTitle.setFillColor(sf::Color(255, 95, 105));
        joinTitle.setPosition(sf::Vector2f(rightX + 24.0f, cardY + 20.0f));
        window.draw(joinTitle);

        sf::Text joinDesc(*m_font, 
            "Connects to an existing host on LAN or direct IP.\n"
            "You play as Player 2 using standard keyboard controls!", 12);
        joinDesc.setFillColor(sf::Color(190, 205, 225));
        joinDesc.setPosition(sf::Vector2f(rightX + 24.0f, cardY + 54.0f));
        window.draw(joinDesc);

        // Target IP Input Field
        sf::Text targetIpLabel(*m_font, "TARGET HOST IP ADDRESS:", 11);
        targetIpLabel.setStyle(sf::Text::Bold);
        targetIpLabel.setFillColor(sf::Color(255, 215, 60));
        targetIpLabel.setPosition(sf::Vector2f(rightX + 24.0f, cardY + 115.0f));
        window.draw(targetIpLabel);

        sf::FloatRect ipInputRect(sf::Vector2f(rightX + 24.0f, cardY + 138.0f), sf::Vector2f(470.0f, 44.0f));
        bool ipHover = ipInputRect.contains(m_mousePos);
        sf::RectangleShape ipInputBg(sf::Vector2f(470.0f, 44.0f));
        ipInputBg.setPosition(sf::Vector2f(rightX + 24.0f, cardY + 138.0f));
        ipInputBg.setFillColor(sf::Color(12, 16, 24));
        if (m_ipInputFocused) {
            ipInputBg.setOutlineColor(sf::Color(255, 215, 60));
            ipInputBg.setOutlineThickness(2.5f);
        } else {
            ipInputBg.setOutlineColor(ipHover ? sf::Color(120, 150, 190) : sf::Color(55, 75, 105));
            ipInputBg.setOutlineThickness(1.5f);
        }
        window.draw(ipInputBg);

        float blink = (std::sin(m_animTime * 7.0f) + 1.0f) * 0.5f;
        std::string ipDisplayStr = m_joinIpInput + (m_ipInputFocused && blink > 0.4f ? "_" : "");
        sf::Text ipInputText(*m_font, ipDisplayStr.empty() ? "Click to enter Host IP (e.g. 127.0.0.1)" : ipDisplayStr, 16);
        ipInputText.setStyle(sf::Text::Bold);
        ipInputText.setFillColor(m_joinIpInput.empty() ? sf::Color(110, 125, 145) : sf::Color(255, 255, 255));
        ipInputText.setPosition(sf::Vector2f(rightX + 38.0f, cardY + 148.0f));
        window.draw(ipInputText);

        sf::Text joinHelp(*m_font, 
            "Quick Testing: Use 127.0.0.1 to play against 2nd instance locally!\n"
            "Press [Enter] or Click to Connect. Paste with [Ctrl+V].", 11);
        joinHelp.setFillColor(sf::Color(140, 160, 185));
        joinHelp.setPosition(sf::Vector2f(rightX + 24.0f, cardY + 205.0f));
        window.draw(joinHelp);

        // Join Button
        bool isClientActive = (m_netManager && (m_netManager->getStatus() == ConnectionStatus::Connecting || (m_netManager->isClient() && m_netManager->isConnected())));
        sf::FloatRect joinBtnRect(sf::Vector2f(rightX + 24.0f, cardY + 305.0f), sf::Vector2f(470.0f, 48.0f));
        bool joinHover = joinBtnRect.contains(m_mousePos);

        sf::RectangleShape joinBtn(sf::Vector2f(470.0f, 48.0f));
        joinBtn.setPosition(sf::Vector2f(rightX + 24.0f, cardY + 305.0f));
        if (isClientActive) {
            joinBtn.setFillColor(joinHover ? sf::Color(230, 60, 60) : sf::Color(190, 40, 40));
            joinBtn.setOutlineColor(sf::Color(255, 120, 120));
        } else {
            joinBtn.setFillColor(joinHover ? sf::Color(50, 190, 110) : sf::Color(35, 150, 85));
            joinBtn.setOutlineColor(joinHover ? sf::Color(255, 215, 60) : sf::Color(80, 220, 130));
        }
        joinBtn.setOutlineThickness(joinHover ? 2.5f : 1.5f);
        window.draw(joinBtn);

        sf::Text joinBtnText(*m_font, isClientActive ? "DISCONNECT" : "CONNECT TO HOST", 15);
        joinBtnText.setStyle(sf::Text::Bold);
        joinBtnText.setFillColor(sf::Color::White);
        joinBtnText.setOrigin(sf::Vector2f(joinBtnText.getLocalBounds().size.x * 0.5f, joinBtnText.getLocalBounds().size.y * 0.5f + 2.0f));
        joinBtnText.setPosition(sf::Vector2f(rightX + 24.0f + 235.0f, cardY + 329.0f));
        window.draw(joinBtnText);

        // ---------------------------------------------------------------------
        // BOTTOM ACTION ROW: PROCEED & BACK
        // ---------------------------------------------------------------------
        // Back Button
        sf::FloatRect backBtnRect(sf::Vector2f(250.0f, 660.0f), sf::Vector2f(220.0f, 46.0f));
        bool backHover = backBtnRect.contains(m_mousePos);
        sf::RectangleShape backBtn(sf::Vector2f(220.0f, 46.0f));
        backBtn.setPosition(sf::Vector2f(250.0f, 660.0f));
        backBtn.setFillColor(backHover ? sf::Color(45, 55, 75) : sf::Color(26, 32, 45));
        backBtn.setOutlineColor(backHover ? sf::Color(255, 215, 60) : sf::Color(70, 85, 115));
        backBtn.setOutlineThickness(backHover ? 2.5f : 1.5f);
        window.draw(backBtn);

        sf::Text backText(*m_font, "< BACK TO MENU", 14);
        backText.setStyle(sf::Text::Bold);
        backText.setFillColor(backHover ? sf::Color(255, 215, 60) : sf::Color(200, 215, 235));
        backText.setOrigin(sf::Vector2f(backText.getLocalBounds().size.x * 0.5f, backText.getLocalBounds().size.y * 0.5f + 2.0f));
        backText.setPosition(sf::Vector2f(360.0f, 683.0f));
        window.draw(backText);

        // Proceed Button (Only active when Connected)
        if (m_netManager && m_netManager->isConnected()) {
            float procPulse = (std::sin(m_animTime * 6.0f) + 1.0f) * 0.5f;
            sf::FloatRect procBtnRect(sf::Vector2f(500.0f, 655.0f), sf::Vector2f(600.0f, 54.0f));
            bool procHover = procBtnRect.contains(m_mousePos);

            sf::RectangleShape procBtn(sf::Vector2f(600.0f, 54.0f));
            procBtn.setPosition(sf::Vector2f(500.0f, 655.0f));
            procBtn.setFillColor(procHover ? sf::Color(40, 200, 110) : sf::Color(25, 160, 85));
            procBtn.setOutlineColor(sf::Color(255, 215, 60));
            procBtn.setOutlineThickness(2.5f + procPulse * 1.5f);
            window.draw(procBtn);

            sf::Text procText(*m_font, "PROCEED TO CHARACTER SELECT  >>", 17);
            procText.setStyle(sf::Text::Bold);
            procText.setFillColor(sf::Color::White);
            procText.setOrigin(sf::Vector2f(procText.getLocalBounds().size.x * 0.5f, procText.getLocalBounds().size.y * 0.5f + 2.0f));
            procText.setPosition(sf::Vector2f(800.0f, 682.0f));
            window.draw(procText);
        }
    }

    // -------------------------------------------------------------------------
    // 2. CPU COMING SOON MODAL
    // -------------------------------------------------------------------------
    void drawCpuComingSoonModal(sf::RenderWindow& window) {
        // Overlay mask
        sf::RectangleShape mask(sf::Vector2f(1600.0f, 900.0f));
        mask.setFillColor(sf::Color(8, 10, 16, 220));
        window.draw(mask);

        // Modal Frame
        sf::RectangleShape frame(sf::Vector2f(620.0f, 350.0f));
        frame.setOrigin(sf::Vector2f(310.0f, 175.0f));
        frame.setPosition(sf::Vector2f(800.0f, 450.0f));
        frame.setFillColor(sf::Color(18, 22, 32, 250));
        frame.setOutlineColor(sf::Color(255, 200, 40));
        frame.setOutlineThickness(3.0f);
        window.draw(frame);

        // Header Title
        sf::Text header(*m_font, "VS CPU // SINGLE PLAYER MODE", 24);
        header.setStyle(sf::Text::Bold);
        header.setFillColor(sf::Color(255, 215, 60));
        header.setOrigin(sf::Vector2f(header.getLocalBounds().size.x * 0.5f, 0.0f));
        header.setPosition(sf::Vector2f(800.0f, 305.0f));
        window.draw(header);

        // Status Badge
        sf::RectangleShape badge(sf::Vector2f(200.0f, 26.0f));
        badge.setOrigin(sf::Vector2f(100.0f, 13.0f));
        badge.setPosition(sf::Vector2f(800.0f, 352.0f));
        badge.setFillColor(sf::Color(220, 45, 45));
        window.draw(badge);

        sf::Text badgeText(*m_font, "UNDER DEVELOPMENT", 11);
        badgeText.setStyle(sf::Text::Bold);
        badgeText.setFillColor(sf::Color::White);
        badgeText.setOrigin(sf::Vector2f(badgeText.getLocalBounds().size.x * 0.5f, badgeText.getLocalBounds().size.y * 0.5f + 2.0f));
        badgeText.setPosition(sf::Vector2f(800.0f, 352.0f));
        window.draw(badgeText);

        // Message
        sf::Text msg(*m_font, 
            "The Stickmin AI Fighter brain is currently undergoing advanced combat training\n"
            "at the Toppat orbital station! Adaptive difficulty algorithms, full arcade ladder,\n"
            "and boss encounter modes will be available in the upcoming update.\n\n"
            "Grab a friend for local Versus, or hone your moves in Practice mode!", 14);
        msg.setFillColor(sf::Color(200, 215, 235));
        msg.setOrigin(sf::Vector2f(msg.getLocalBounds().size.x * 0.5f, 0.0f));
        msg.setPosition(sf::Vector2f(800.0f, 395.0f));
        window.draw(msg);

        // OK Button
        sf::FloatRect okRect(sf::Vector2f(670.0f, 540.0f), sf::Vector2f(260.0f, 46.0f));
        bool okHover = okRect.contains(m_mousePos);
        sf::RectangleShape okBtn(sf::Vector2f(260.0f, 46.0f));
        okBtn.setOrigin(sf::Vector2f(130.0f, 23.0f));
        okBtn.setPosition(sf::Vector2f(800.0f, 563.0f));
        okBtn.setFillColor(okHover ? sf::Color(65, 150, 245) : sf::Color(45, 120, 220));
        okBtn.setOutlineColor(okHover ? sf::Color(255, 235, 100) : sf::Color(255, 255, 255));
        okBtn.setOutlineThickness(okHover ? 3.0f : 2.0f);
        window.draw(okBtn);

        sf::Text okText(*m_font, "OK // BACK TO MENU", 15);
        okText.setStyle(sf::Text::Bold);
        okText.setFillColor(okHover ? sf::Color(255, 255, 120) : sf::Color::White);
        okText.setOrigin(sf::Vector2f(okText.getLocalBounds().size.x * 0.5f, okText.getLocalBounds().size.y * 0.5f + 3.0f));
        okText.setPosition(sf::Vector2f(800.0f, 563.0f));
        window.draw(okText);
    }

    // -------------------------------------------------------------------------
    // 3. COMMAND LIST MODAL
    // -------------------------------------------------------------------------
    void drawCommandListModal(sf::RenderWindow& window) {
        sf::RectangleShape mask(sf::Vector2f(1600.0f, 900.0f));
        mask.setFillColor(sf::Color(8, 10, 16, 235));
        window.draw(mask);

        // Large Modal Box
        sf::RectangleShape box(sf::Vector2f(1100.0f, 750.0f));
        box.setOrigin(sf::Vector2f(550.0f, 375.0f));
        box.setPosition(sf::Vector2f(800.0f, 450.0f));
        box.setFillColor(sf::Color(16, 20, 28, 250));
        box.setOutlineColor(sf::Color(45, 145, 255));
        box.setOutlineThickness(3.0f);
        window.draw(box);

        // Title
        sf::Text title(*m_font, "COMMAND LIST & FIGHTING GAME MECHANICS", 24);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(255, 255, 255));
        title.setOutlineColor(sf::Color(45, 145, 255));
        title.setOutlineThickness(2.0f);
        title.setOrigin(sf::Vector2f(title.getLocalBounds().size.x * 0.5f, 0.0f));
        title.setPosition(sf::Vector2f(800.0f, 95.0f));
        window.draw(title);

        sf::Text sub(*m_font, "TEKKEN 4-BUTTON SYSTEM", 13);
        sub.setFillColor(sf::Color(255, 215, 60));
        sub.setOrigin(sf::Vector2f(sub.getLocalBounds().size.x * 0.5f, 0.0f));
        sub.setPosition(sf::Vector2f(800.0f, 132.0f));
        window.draw(sub);

        // 3 Column Layout
        float colY = 175.0f;
        float colW = 330.0f;

        // Column 1: Basic 4-Button Controls
        sf::RectangleShape col1(sf::Vector2f(colW, 530.0f));
        col1.setPosition(sf::Vector2f(280.0f, colY));
        col1.setFillColor(sf::Color(22, 28, 40));
        col1.setOutlineColor(sf::Color(55, 70, 95));
        col1.setOutlineThickness(1.5f);
        window.draw(col1);

        sf::Text h1(*m_font, "1. FOUR-BUTTON LAYOUT", 15);
        h1.setStyle(sf::Text::Bold);
        h1.setFillColor(sf::Color(80, 180, 255));
        h1.setPosition(sf::Vector2f(295.0f, colY + 14.0f));
        window.draw(h1);

        sf::Text t1(*m_font,
            "PLAYER 1 CONTROLS:\n"
            "  1 (LP): [J]       2 (RP): [K]\n"
            "  3 (LK): [U]       4 (RK): [I]\n"
            "  Move: [W] [A] [S] [D]\n\n"
            "PLAYER 2 CONTROLS:\n"
            "  1 (LP): [Num1]   2 (RP): [Num2]\n"
            "  3 (LK): [Num4]   4 (RK): [Num5]\n"
            "  Move: [Arrow Keys]\n\n"
            "NATURAL COMBOS:\n"
            "  1, 2 String : [J] -> [K]\n"
            "  Fast double strike, +frame on hit.\n\n"
            "DASHING:\n"
            "  Double tap [D] or [A] to dash.\n"
            "  Covers ground with forward momentum.", 13);
        t1.setFillColor(sf::Color(215, 225, 240));
        t1.setPosition(sf::Vector2f(295.0f, colY + 45.0f));
        window.draw(t1);

        // Column 2: Universal Special Moves
        sf::RectangleShape col2(sf::Vector2f(colW, 530.0f));
        col2.setPosition(sf::Vector2f(635.0f, colY));
        col2.setFillColor(sf::Color(22, 28, 40));
        col2.setOutlineColor(sf::Color(55, 70, 95));
        col2.setOutlineThickness(1.5f);
        window.draw(col2);

        sf::Text h2(*m_font, "2. SPECIAL MOVES", 15);
        h2.setStyle(sf::Text::Bold);
        h2.setFillColor(sf::Color(255, 205, 60));
        h2.setPosition(sf::Vector2f(650.0f, colY + 14.0f));
        window.draw(h2);

        sf::Text t2(*m_font,
            "ELECTRIC WIND GOD FIST (EWGF):\n"
            "  [Forward + 2] (D+K / Right+Num2)\n"
            "  High launcher with lightning sparks!\n\n"
            "HELL SWEEP:\n"
            "  [Down + 3] (S+U / Down+Num4)\n"
            "  Low trip kick that floors opponent.\n\n"
            "HOPKICK:\n"
            "  [Up + 4] (W+I / Up+Num5)\n"
            "  Crushes low attacks, air launcher!\n\n"
            "POWER CRUSH:\n"
            "  [1+2] (Key [O] or J+K / Num3)\n"
            "  Absorbs hits with SUPER ARMOR.\n\n"
            "COMMAND THROW:\n"
            "  [1+3] (Key [L] or J+U / Num6)\n"
            "  Unblockable grappling slam!", 13);
        t2.setFillColor(sf::Color(215, 225, 240));
        t2.setPosition(sf::Vector2f(650.0f, colY + 45.0f));
        window.draw(t2);

        // Column 3: Defense & Rage Art
        sf::RectangleShape col3(sf::Vector2f(colW, 530.0f));
        col3.setPosition(sf::Vector2f(990.0f, colY));
        col3.setFillColor(sf::Color(22, 28, 40));
        col3.setOutlineColor(sf::Color(55, 70, 95));
        col3.setOutlineThickness(1.5f);
        window.draw(col3);

        sf::Text h3(*m_font, "3. DEFENSE & RAGE", 15);
        h3.setStyle(sf::Text::Bold);
        h3.setFillColor(sf::Color(255, 75, 85));
        h3.setPosition(sf::Vector2f(1005.0f, colY + 14.0f));
        window.draw(h3);

        sf::Text t3(*m_font,
            "HIGH GUARD (BLOCK):\n"
            "  Hold [Back] (away from opponent).\n"
            "  Blocks high and mid attacks.\n\n"
            "CROUCH GUARD:\n"
            "  Hold [Down + Back].\n"
            "  Blocks lows, ducks high punches.\n\n"
            "TECH ROLL (GETUP):\n"
            "  Tap any attack button on floor\n"
            "  to instantly roll back to guard.\n\n"
            "RAGE ART FINISHER:\n"
            "  Active when HP < 25% (Red Glow)!\n"
            "  Press [1+2] ([O] or [Num3])\n"
            "  Cinematic ultra attack!\n\n"
            "WALL SPLATS:\n"
            "  Smack opponents into arena walls\n"
            "  for high-damage wall crumples!", 13);
        t3.setFillColor(sf::Color(215, 225, 240));
        t3.setPosition(sf::Vector2f(1005.0f, colY + 45.0f));
        window.draw(t3);

        // Close Button
        sf::FloatRect closeRect(sf::Vector2f(670.0f, 728.0f), sf::Vector2f(260.0f, 44.0f));
        bool closeHover = closeRect.contains(m_mousePos);
        sf::RectangleShape closeBtn(sf::Vector2f(260.0f, 44.0f));
        closeBtn.setOrigin(sf::Vector2f(130.0f, 22.0f));
        closeBtn.setPosition(sf::Vector2f(800.0f, 750.0f));
        closeBtn.setFillColor(closeHover ? sf::Color(65, 150, 245) : sf::Color(45, 120, 220));
        closeBtn.setOutlineColor(closeHover ? sf::Color(255, 235, 100) : sf::Color::White);
        closeBtn.setOutlineThickness(closeHover ? 3.0f : 2.0f);
        window.draw(closeBtn);

        sf::Text closeText(*m_font, "[ CLOSE // ESC ]", 15);
        closeText.setStyle(sf::Text::Bold);
        closeText.setFillColor(closeHover ? sf::Color(255, 255, 120) : sf::Color::White);
        closeText.setOrigin(sf::Vector2f(closeText.getLocalBounds().size.x * 0.5f, closeText.getLocalBounds().size.y * 0.5f + 3.0f));
        closeText.setPosition(sf::Vector2f(800.0f, 750.0f));
        window.draw(closeText);
    }

    // -------------------------------------------------------------------------
    // 4. CHARACTER SELECT (TEKKEN 8 STYLE)
    // -------------------------------------------------------------------------
    void drawCharacterSelect(sf::RenderWindow& window) {
        const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();

        // Dark background
        sf::RectangleShape bg(sf::Vector2f(1600.0f, 900.0f));
        bg.setFillColor(sf::Color(12, 14, 20));
        window.draw(bg);

        // Header
        sf::Text header(*m_font, "SELECT YOUR FIGHTER", 34);
        header.setStyle(sf::Text::Bold);
        header.setFillColor(sf::Color::White);
        header.setOutlineColor(sf::Color(45, 145, 255));
        header.setOutlineThickness(3.0f);
        header.setOrigin(sf::Vector2f(header.getLocalBounds().size.x * 0.5f, 0.0f));
        header.setPosition(sf::Vector2f(800.0f, 30.0f));
        window.draw(header);

        std::string subStr = "CLICK [PICK P1] / [PICK P2] ON CARDS OR PRESS J / ENTER TO LOCK IN";
        if (m_gameMode == GameMode::Online && m_netManager) {
            if (m_netManager->isHost()) {
                subStr = "ONLINE MATCH // YOU ARE PLAYER 1 (HOST) - CHOOSE FIGHTER [J / SPACE TO LOCK]";
            } else if (m_netManager->isClient()) {
                int pingVal = static_cast<int>(m_netManager->getPingMs());
                subStr = "ONLINE MATCH // YOU ARE PLAYER 2 (CLIENT) | PING: " + std::to_string(pingVal) + " ms - [J / SPACE TO LOCK]";
            }
        }
        sf::Text sub(*m_font, subStr, 12);
        sub.setFillColor(sf::Color(255, 215, 60));
        sub.setOrigin(sf::Vector2f(sub.getLocalBounds().size.x * 0.5f, 0.0f));
        sub.setPosition(sf::Vector2f(800.0f, 76.0f));
        window.draw(sub);

        // ---------------------------------------------------------------------
        // Center: Character Selection Cards
        // ---------------------------------------------------------------------
        float startX = 415.0f;
        float cardW = 145.0f;
        float cardH = 200.0f;
        float cardGap = 16.0f;

        for (size_t i = 0; i < roster.size(); ++i) {
            const auto& def = roster[i];
            float cx = startX + i * (cardW + cardGap);
            float cy = 250.0f;

            bool isP1 = (m_p1CharIdx == i);
            bool isP2 = (m_p2CharIdx == i);
            bool isCardHovered = sf::FloatRect(sf::Vector2f(cx, cy), sf::Vector2f(cardW, cardH)).contains(m_mousePos);

            sf::RectangleShape card(sf::Vector2f(cardW, cardH));
            card.setPosition(sf::Vector2f(cx, cy));
            card.setFillColor(isCardHovered ? sf::Color(26, 32, 46, 250) : sf::Color(20, 24, 34, 240));

            if (isP1 && isP2) {
                card.setOutlineColor(sf::Color(255, 215, 60)); // Gold if both pick same
                card.setOutlineThickness(3.5f);
            } else if (isP1) {
                card.setOutlineColor(sf::Color(45, 145, 255)); // Cyan for P1
                card.setOutlineThickness(3.5f);
            } else if (isP2) {
                card.setOutlineColor(sf::Color(245, 55, 65));  // Crimson for P2
                card.setOutlineThickness(3.5f);
            } else if (isCardHovered) {
                card.setOutlineColor(sf::Color(100, 130, 175));
                card.setOutlineThickness(2.2f);
            } else {
                card.setOutlineColor(sf::Color(55, 65, 85));
                card.setOutlineThickness(1.5f);
            }
            window.draw(card);

            // Portrait in Card
            if (m_ragdollRenderer) {
                m_ragdollRenderer->drawPortrait(window, sf::Vector2f(cx + cardW * 0.5f, cy + 65.0f), 26.0f, 1, def);
            }

            // Name
            sf::Text name(*m_font, def.displayName, 11);
            name.setStyle(sf::Text::Bold);
            name.setFillColor(def.accentColor);
            name.setOrigin(sf::Vector2f(name.getLocalBounds().size.x * 0.5f, 0.0f));
            name.setPosition(sf::Vector2f(cx + cardW * 0.5f, cy + 130.0f));
            window.draw(name);

            // Title
            sf::Text title(*m_font, def.title, 9);
            title.setFillColor(sf::Color(160, 175, 195));
            title.setOrigin(sf::Vector2f(title.getLocalBounds().size.x * 0.5f, 0.0f));
            title.setPosition(sf::Vector2f(cx + cardW * 0.5f, cy + 152.0f));
            window.draw(title);

            // Interactive P1 / P2 Badges
            sf::FloatRect p1TagRect(sf::Vector2f(cx + 6.0f, cy + 6.0f), sf::Vector2f(56.0f, 20.0f));
            bool p1Hover = p1TagRect.contains(m_mousePos);
            sf::RectangleShape p1Pill(sf::Vector2f(56.0f, 20.0f));
            p1Pill.setPosition(sf::Vector2f(cx + 6.0f, cy + 6.0f));
            p1Pill.setFillColor(isP1 ? sf::Color(45, 145, 255) : (p1Hover ? sf::Color(35, 65, 110) : sf::Color(22, 28, 40)));
            p1Pill.setOutlineColor(isP1 ? sf::Color::White : (p1Hover ? sf::Color(65, 165, 255) : sf::Color(48, 62, 82)));
            p1Pill.setOutlineThickness(isP1 || p1Hover ? 1.5f : 1.0f);
            window.draw(p1Pill);

            sf::Text p1Tag(*m_font, isP1 ? "P1 LOCK" : "PICK P1", 9);
            p1Tag.setStyle(sf::Text::Bold);
            p1Tag.setFillColor(isP1 ? sf::Color::White : (p1Hover ? sf::Color(140, 205, 255) : sf::Color(135, 150, 170)));
            p1Tag.setOrigin(sf::Vector2f(p1Tag.getLocalBounds().size.x * 0.5f, p1Tag.getLocalBounds().size.y * 0.5f + 2.0f));
            p1Tag.setPosition(sf::Vector2f(cx + 34.0f, cy + 15.0f));
            window.draw(p1Tag);

            sf::FloatRect p2TagRect(sf::Vector2f(cx + cardW - 62.0f, cy + 6.0f), sf::Vector2f(56.0f, 20.0f));
            bool p2Hover = p2TagRect.contains(m_mousePos);
            sf::RectangleShape p2Pill(sf::Vector2f(56.0f, 20.0f));
            p2Pill.setPosition(sf::Vector2f(cx + cardW - 62.0f, cy + 6.0f));
            p2Pill.setFillColor(isP2 ? sf::Color(245, 55, 65) : (p2Hover ? sf::Color(110, 35, 45) : sf::Color(22, 28, 40)));
            p2Pill.setOutlineColor(isP2 ? sf::Color::White : (p2Hover ? sf::Color(255, 75, 85) : sf::Color(82, 48, 52)));
            p2Pill.setOutlineThickness(isP2 || p2Hover ? 1.5f : 1.0f);
            window.draw(p2Pill);

            sf::Text p2Tag(*m_font, isP2 ? "P2 LOCK" : "PICK P2", 9);
            p2Tag.setStyle(sf::Text::Bold);
            p2Tag.setFillColor(isP2 ? sf::Color::White : (p2Hover ? sf::Color(255, 150, 160) : sf::Color(170, 135, 140)));
            p2Tag.setOrigin(sf::Vector2f(p2Tag.getLocalBounds().size.x * 0.5f, p2Tag.getLocalBounds().size.y * 0.5f + 2.0f));
            p2Tag.setPosition(sf::Vector2f(cx + cardW - 34.0f, cy + 15.0f));
            window.draw(p2Tag);
        }

        // ---------------------------------------------------------------------
        // Left: Player 1 Profile Card
        // ---------------------------------------------------------------------
        drawPlayerProfileCard(window, 1, m_p1CharIdx, m_p1Ready, sf::Vector2f(60.0f, 130.0f), sf::Vector2f(330.0f, 620.0f));

        // ---------------------------------------------------------------------
        // Right: Player 2 Profile Card
        // ---------------------------------------------------------------------
        drawPlayerProfileCard(window, 2, m_p2CharIdx, m_p2Ready, sf::Vector2f(1210.0f, 130.0f), sf::Vector2f(330.0f, 620.0f));

        // Back Button
        sf::FloatRect backRect(sf::Vector2f(40.0f, 30.0f), sf::Vector2f(130.0f, 34.0f));
        bool backHover = backRect.contains(m_mousePos);
        sf::RectangleShape backBtn(sf::Vector2f(130.0f, 34.0f));
        backBtn.setPosition(sf::Vector2f(40.0f, 30.0f));
        backBtn.setFillColor(backHover ? sf::Color(38, 46, 62) : sf::Color(24, 28, 38));
        backBtn.setOutlineColor(backHover ? sf::Color(255, 215, 60) : sf::Color(65, 80, 105));
        backBtn.setOutlineThickness(backHover ? 2.5f : 1.5f);
        window.draw(backBtn);

        sf::Text backText(*m_font, "< BACK", 12);
        backText.setStyle(sf::Text::Bold);
        backText.setFillColor(backHover ? sf::Color::White : sf::Color(180, 195, 215));
        backText.setPosition(sf::Vector2f(72.0f, 38.0f));
        window.draw(backText);

        // Flash banner when both ready
        if (m_p1Ready && m_p2Ready) {
            sf::RectangleShape readyBanner(sf::Vector2f(1600.0f, 60.0f));
            readyBanner.setPosition(sf::Vector2f(0.0f, 480.0f));
            readyBanner.setFillColor(sf::Color(255, 215, 60, 220));
            window.draw(readyBanner);

            sf::Text readyText(*m_font, "FIGHTERS READY! COMMENCING STAGE SELECTION...", 22);
            readyText.setStyle(sf::Text::Bold);
            readyText.setFillColor(sf::Color(10, 12, 16));
            readyText.setOrigin(sf::Vector2f(readyText.getLocalBounds().size.x * 0.5f, readyText.getLocalBounds().size.y * 0.5f + 4.0f));
            readyText.setPosition(sf::Vector2f(800.0f, 510.0f));
            window.draw(readyText);
        }
    }

    void drawPlayerProfileCard(sf::RenderWindow& window, int playerNum, size_t charIdx, bool isReady, const sf::Vector2f& pos, const sf::Vector2f& size) {
        const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();
        const auto& def = roster[charIdx];

        sf::Color themeCol = (playerNum == 1) ? sf::Color(45, 145, 255) : sf::Color(245, 55, 65);

        sf::RectangleShape box(size);
        box.setPosition(pos);
        box.setFillColor(sf::Color(16, 20, 28, 245));
        box.setOutlineColor(isReady ? sf::Color(255, 215, 60) : themeCol);
        box.setOutlineThickness(isReady ? 3.5f : 2.0f);
        window.draw(box);

        // Header Tag
        sf::Text tag(*m_font, playerNum == 1 ? "PLAYER 1" : "PLAYER 2", 14);
        tag.setStyle(sf::Text::Bold);
        tag.setFillColor(themeCol);
        tag.setPosition(sf::Vector2f(pos.x + 20.0f, pos.y + 16.0f));
        window.draw(tag);

        // Name
        sf::Text name(*m_font, def.displayName, 20);
        name.setStyle(sf::Text::Bold);
        name.setFillColor(sf::Color::White);
        name.setPosition(sf::Vector2f(pos.x + 20.0f, pos.y + 38.0f));
        window.draw(name);

        // Title
        sf::Text title(*m_font, def.title, 12);
        title.setFillColor(def.accentColor);
        title.setPosition(sf::Vector2f(pos.x + 20.0f, pos.y + 66.0f));
        window.draw(title);

        // Large Portrait Box
        sf::RectangleShape pBox(sf::Vector2f(size.x - 40.0f, 150.0f));
        pBox.setPosition(sf::Vector2f(pos.x + 20.0f, pos.y + 95.0f));
        pBox.setFillColor(sf::Color(10, 12, 16));
        pBox.setOutlineColor(sf::Color(40, 50, 65));
        pBox.setOutlineThickness(1.5f);
        window.draw(pBox);

        if (m_ragdollRenderer) {
            float portraitX = pos.x + size.x * 0.5f;
            float portraitY = pos.y + 170.0f;
            m_ragdollRenderer->drawPortrait(window, sf::Vector2f(portraitX, portraitY), 38.0f, (playerNum == 1 ? 1 : -1), def);
        }

        // Stats Meters
        float statY = pos.y + 265.0f;
        auto drawStatBar = [&](const std::string& label, float value, sf::Color col) {
            sf::Text lbl(*m_font, label, 11);
            lbl.setStyle(sf::Text::Bold);
            lbl.setFillColor(sf::Color(180, 195, 215));
            lbl.setPosition(sf::Vector2f(pos.x + 22.0f, statY));
            window.draw(lbl);

            sf::RectangleShape barBg(sf::Vector2f(size.x - 44.0f, 12.0f));
            barBg.setPosition(sf::Vector2f(pos.x + 22.0f, statY + 18.0f));
            barBg.setFillColor(sf::Color(25, 30, 42));
            window.draw(barBg);

            sf::RectangleShape barFill(sf::Vector2f((size.x - 44.0f) * value, 12.0f));
            barFill.setPosition(sf::Vector2f(pos.x + 22.0f, statY + 18.0f));
            barFill.setFillColor(col);
            window.draw(barFill);

            statY += 38.0f;
        };

        // Custom stats per character
        float pwr = (charIdx == 4 ? 0.95f : charIdx == 3 ? 0.85f : charIdx == 2 ? 0.80f : charIdx == 1 ? 0.70f : 0.75f);
        float spd = (charIdx == 1 ? 0.95f : charIdx == 0 ? 0.80f : charIdx == 3 ? 0.75f : charIdx == 2 ? 0.70f : 0.60f);
        float defStat = (charIdx == 4 ? 0.95f : charIdx == 3 ? 0.85f : charIdx == 2 ? 0.80f : charIdx == 0 ? 0.75f : 0.65f);

        drawStatBar("POWER / DAMAGE", pwr, sf::Color(245, 75, 75));
        drawStatBar("SPEED / AGILITY", spd, sf::Color(65, 205, 120));
        drawStatBar("DEFENSE / ARMOR", defStat, sf::Color(75, 160, 245));

        // Signature Moves summary
        std::string moveDesc = "";
        if (charIdx == 0) moveDesc = "Balanced all-rounder.\nMishima EWGF & Hell Sweep.";
        else if (charIdx == 1) moveDesc = "High agility & fast pokes.\nHopkick air juggle specialist.";
        else if (charIdx == 2) moveDesc = "Heavy impact momentum.\nHigh damage flying dropkick.";
        else if (charIdx == 3) moveDesc = "Counter-striking aristocrat.\nHeavy Power Crush armor.";
        else if (charIdx == 4) moveDesc = "Cyborg juggernaut.\nMassive hits, armored strikes.";

        sf::Text moves(*m_font, moveDesc, 11);
        moves.setFillColor(sf::Color(190, 205, 225));
        moves.setPosition(sf::Vector2f(pos.x + 22.0f, statY + 5.0f));
        window.draw(moves);

        // Ready Status Box
        sf::FloatRect btnRect(sf::Vector2f(pos.x + 22.0f, pos.y + size.y - 60.0f), sf::Vector2f(size.x - 44.0f, 44.0f));
        bool btnHover = btnRect.contains(m_mousePos);
        sf::RectangleShape statusBtn(sf::Vector2f(size.x - 44.0f, 44.0f));
        statusBtn.setPosition(sf::Vector2f(pos.x + 22.0f, pos.y + size.y - 60.0f));
        if (isReady) {
            statusBtn.setFillColor(btnHover ? sf::Color(55, 210, 105) : sf::Color(45, 180, 85));
            statusBtn.setOutlineColor(btnHover ? sf::Color(255, 255, 120) : sf::Color(255, 255, 255));
            statusBtn.setOutlineThickness(btnHover ? 3.0f : 2.0f);
        } else {
            statusBtn.setFillColor(btnHover ? sf::Color(45, 55, 75) : sf::Color(30, 36, 48));
            statusBtn.setOutlineColor(btnHover ? sf::Color(255, 215, 60) : themeCol);
            statusBtn.setOutlineThickness(btnHover ? 2.5f : 1.5f);
        }
        window.draw(statusBtn);

        std::string statusStr = isReady ? "READY! [LOCKED IN]" : (playerNum == 1 ? "P1: PRESS [J] TO LOCK" : (m_gameMode == GameMode::Online ? "P2: PRESS [J] TO LOCK" : "P2: PRESS [ENTER] TO LOCK"));
        sf::Text statusText(*m_font, statusStr, 13);
        statusText.setStyle(sf::Text::Bold);
        statusText.setFillColor(sf::Color::White);
        statusText.setOrigin(sf::Vector2f(statusText.getLocalBounds().size.x * 0.5f, statusText.getLocalBounds().size.y * 0.5f + 3.0f));
        statusText.setPosition(sf::Vector2f(pos.x + size.x * 0.5f, pos.y + size.y - 38.0f));
        window.draw(statusText);
    }

    // -------------------------------------------------------------------------
    // 5. STAGE SELECT
    // -------------------------------------------------------------------------
    void drawStageSelect(sf::RenderWindow& window) {
        // Real-time stage background is drawn behind by StageRenderer!

        // Dark dimming plate for readability
        sf::RectangleShape plate(sf::Vector2f(1600.0f, 900.0f));
        plate.setFillColor(sf::Color(10, 12, 18, 160));
        window.draw(plate);

        // Header
        sf::Text header(*m_font, "SELECT BATTLEGROUND", 34);
        header.setStyle(sf::Text::Bold);
        header.setFillColor(sf::Color::White);
        header.setOutlineColor(sf::Color(255, 215, 60));
        header.setOutlineThickness(3.0f);
        header.setOrigin(sf::Vector2f(header.getLocalBounds().size.x * 0.5f, 0.0f));
        header.setPosition(sf::Vector2f(800.0f, 35.0f));
        window.draw(header);

        bool isOnlineClient = (m_gameMode == GameMode::Online && m_netManager && m_netManager->isClient());

        if (isOnlineClient) {
            sf::Text sub(*m_font, "HOST IS CHOOSING BATTLEGROUND // WAITING FOR HOST TO COMMENCE...", 13);
            sub.setStyle(sf::Text::Bold);
            sub.setFillColor(sf::Color(80, 220, 255));
            sub.setOrigin(sf::Vector2f(sub.getLocalBounds().size.x * 0.5f, 0.0f));
            sub.setPosition(sf::Vector2f(800.0f, 82.0f));
            window.draw(sub);
        } else {
            sf::Text sub(*m_font, "CHOOSE ARENA // USE [A/D] OR CLICK TO SELECT // PRESS [ENTER] TO COMMENCE BATTLE", 12);
            sub.setFillColor(sf::Color(255, 215, 60));
            sub.setOrigin(sf::Vector2f(sub.getLocalBounds().size.x * 0.5f, 0.0f));
            sub.setPosition(sf::Vector2f(800.0f, 82.0f));
            window.draw(sub);
        }

        // 3 Stage Cards
        struct StageInfo {
            std::string name;
            std::string sub;
            std::string desc;
            sf::Color color;
        };

        std::vector<StageInfo> stages = {
            { "TOPPAT AIRSHIP DECK", "ALTITUDE: 14,000 FT // SUNSET FLEET", "High-altitude aircraft carrier deck flying through fiery crimson clouds.\nBrisk tailwinds and industrial Toppat steel railings.", sf::Color(245, 55, 65) },
            { "THE WALL - PRISON YARD", "COMPLEX SECTOR 4 // SIBERIAN BLIZZARD", "High-security prison fortress locked in a subzero Siberian blizzard.\nIcy concrete courtyard, searchlight towers, and freezing winds.", sf::Color(80, 200, 255) },
            { "THE BANK VAULT", "DESERT CANYON HEIST // BREACHED SAFES", "Deep desert bank vault blown open with explosive dynamite.\nScattered gold bullion, canyon mesas, and howling dust storms.", sf::Color(255, 195, 45) }
        };

        for (int i = 0; i < 3; ++i) {
            float cx = 170.0f + i * 430.0f;
            float cy = 250.0f;
            float cardW = 390.0f;
            float cardH = 370.0f;

            bool isSelected = (m_stageIdx == i);

            sf::RectangleShape card(sf::Vector2f(cardW, cardH));
            card.setPosition(sf::Vector2f(cx, cy));
            card.setFillColor(sf::Color(16, 20, 30, 230));

            if (isSelected) {
                card.setOutlineColor(stages[i].color);
                card.setOutlineThickness(3.5f);
            } else {
                bool cardHover = !isOnlineClient && sf::FloatRect(sf::Vector2f(cx, cy), sf::Vector2f(cardW, cardH)).contains(m_mousePos);
                card.setOutlineColor(cardHover ? sf::Color(140, 160, 200) : sf::Color(55, 65, 85));
                card.setOutlineThickness(cardHover ? 2.0f : 1.5f);
            }
            window.draw(card);

            // Banner Plate
            sf::RectangleShape topP(sf::Vector2f(cardW, 46.0f));
            topP.setPosition(sf::Vector2f(cx, cy));
            topP.setFillColor(stages[i].color);
            window.draw(topP);

            sf::Text sName(*m_font, stages[i].name, 16);
            sName.setStyle(sf::Text::Bold);
            sName.setFillColor(sf::Color::White);
            sName.setPosition(sf::Vector2f(cx + 18.0f, cy + 12.0f));
            window.draw(sName);

            sf::Text sSub(*m_font, stages[i].sub, 10);
            sSub.setStyle(sf::Text::Bold);
            sSub.setFillColor(stages[i].color);
            sSub.setPosition(sf::Vector2f(cx + 18.0f, cy + 62.0f));
            window.draw(sSub);

            sf::Text sDesc(*m_font, stages[i].desc, 12);
            sDesc.setFillColor(sf::Color(200, 215, 235));
            sDesc.setPosition(sf::Vector2f(cx + 18.0f, cy + 96.0f));
            window.draw(sDesc);

            // Confirmation Pill on Selected
            if (isSelected) {
                sf::FloatRect pillRect(sf::Vector2f(cx + 18.0f, cy + cardH - 62.0f), sf::Vector2f(cardW - 36.0f, 44.0f));
                bool pillHover = !isOnlineClient && pillRect.contains(m_mousePos);
                sf::RectangleShape pill(sf::Vector2f(cardW - 36.0f, 44.0f));
                pill.setPosition(sf::Vector2f(cx + 18.0f, cy + cardH - 62.0f));

                if (isOnlineClient) {
                    pill.setFillColor(sf::Color(16, 28, 44, 220));
                    pill.setOutlineColor(sf::Color(80, 200, 255));
                    pill.setOutlineThickness(1.5f);
                } else {
                    pill.setFillColor(pillHover ? sf::Color(255, 215, 60) : stages[i].color);
                    pill.setOutlineColor(pillHover ? sf::Color::White : sf::Color::Transparent);
                    pill.setOutlineThickness(pillHover ? 2.5f : 0.0f);
                }
                window.draw(pill);

                std::string pillStr = isOnlineClient ? "[ WAITING FOR HOST TO START ]" : "[ PRESS ENTER OR CLICK TO COMMENCE ]";
                sf::Text pillText(*m_font, pillStr, 13);
                pillText.setStyle(sf::Text::Bold);
                pillText.setFillColor(isOnlineClient ? sf::Color(80, 220, 255) : (pillHover ? sf::Color(10, 12, 16) : sf::Color::White));
                pillText.setOrigin(sf::Vector2f(pillText.getLocalBounds().size.x * 0.5f, pillText.getLocalBounds().size.y * 0.5f + 3.0f));
                pillText.setPosition(sf::Vector2f(cx + cardW * 0.5f, cy + cardH - 40.0f));
                window.draw(pillText);
            }
        }

        // Back Button
        sf::FloatRect backRect(sf::Vector2f(40.0f, 30.0f), sf::Vector2f(130.0f, 34.0f));
        bool backHover = backRect.contains(m_mousePos);
        sf::RectangleShape backBtn(sf::Vector2f(130.0f, 34.0f));
        backBtn.setPosition(sf::Vector2f(40.0f, 30.0f));
        backBtn.setFillColor(backHover ? sf::Color(38, 46, 62) : sf::Color(24, 28, 38));
        backBtn.setOutlineColor(backHover ? sf::Color(255, 215, 60) : sf::Color(65, 80, 105));
        backBtn.setOutlineThickness(backHover ? 2.5f : 1.5f);
        window.draw(backBtn);

        sf::Text backText(*m_font, "< BACK", 12);
        backText.setStyle(sf::Text::Bold);
        backText.setFillColor(backHover ? sf::Color::White : sf::Color(180, 195, 215));
        backText.setPosition(sf::Vector2f(72.0f, 38.0f));
        window.draw(backText);
    }

    // -------------------------------------------------------------------------
    // 6. VERSUS INTRO SPLASH ("GET READY FOR THE NEXT BATTLE!")
    // -------------------------------------------------------------------------
    void drawVersusIntro(sf::RenderWindow& window) {
        const auto& roster = RagdollEngine::CharacterRegistry::getAllRosterCharacters();
        const auto& p1Def = roster[m_p1CharIdx];
        const auto& p2Def = roster[m_p2CharIdx];

        // Dark background
        sf::RectangleShape bg(sf::Vector2f(1600.0f, 900.0f));
        bg.setFillColor(sf::Color(10, 12, 16));
        window.draw(bg);

        // Split screen: Diagonal slice
        sf::ConvexShape leftSlice;
        leftSlice.setPointCount(4);
        leftSlice.setPoint(0, sf::Vector2f(0.0f, 0.0f));
        leftSlice.setPoint(1, sf::Vector2f(880.0f, 0.0f));
        leftSlice.setPoint(2, sf::Vector2f(720.0f, 900.0f));
        leftSlice.setPoint(3, sf::Vector2f(0.0f, 900.0f));
        leftSlice.setFillColor(sf::Color(20, 35, 60));
        window.draw(leftSlice);

        sf::ConvexShape rightSlice;
        rightSlice.setPointCount(4);
        rightSlice.setPoint(0, sf::Vector2f(880.0f, 0.0f));
        rightSlice.setPoint(1, sf::Vector2f(1600.0f, 0.0f));
        rightSlice.setPoint(2, sf::Vector2f(1600.0f, 900.0f));
        rightSlice.setPoint(3, sf::Vector2f(720.0f, 900.0f));
        rightSlice.setFillColor(sf::Color(55, 18, 26));
        window.draw(rightSlice);

        // Top Banner
        sf::Text banner(*m_font, "GET READY FOR THE NEXT BATTLE!", 32);
        banner.setStyle(sf::Text::Bold);
        banner.setFillColor(sf::Color(255, 235, 60));
        banner.setOutlineColor(sf::Color::Black);
        banner.setOutlineThickness(3.5f);
        banner.setOrigin(sf::Vector2f(banner.getLocalBounds().size.x * 0.5f, 0.0f));
        banner.setPosition(sf::Vector2f(800.0f, 50.0f));
        window.draw(banner);

        // Player 1 Side
        if (m_ragdollRenderer) {
            m_ragdollRenderer->drawPortrait(window, sf::Vector2f(380.0f, 420.0f), 70.0f, 1, p1Def);
        }

        sf::Text p1Name(*m_font, p1Def.displayName, 32);
        p1Name.setStyle(sf::Text::Bold);
        p1Name.setFillColor(p1Def.accentColor);
        p1Name.setOutlineColor(sf::Color::Black);
        p1Name.setOutlineThickness(3.0f);
        p1Name.setOrigin(sf::Vector2f(p1Name.getLocalBounds().size.x * 0.5f, 0.0f));
        p1Name.setPosition(sf::Vector2f(380.0f, 540.0f));
        window.draw(p1Name);

        sf::Text p1Title(*m_font, p1Def.title, 14);
        p1Title.setStyle(sf::Text::Bold);
        p1Title.setFillColor(sf::Color(180, 205, 235));
        p1Title.setOrigin(sf::Vector2f(p1Title.getLocalBounds().size.x * 0.5f, 0.0f));
        p1Title.setPosition(sf::Vector2f(380.0f, 585.0f));
        window.draw(p1Title);

        // Player 2 Side
        if (m_ragdollRenderer) {
            m_ragdollRenderer->drawPortrait(window, sf::Vector2f(1220.0f, 420.0f), 70.0f, -1, p2Def);
        }

        sf::Text p2Name(*m_font, p2Def.displayName, 32);
        p2Name.setStyle(sf::Text::Bold);
        p2Name.setFillColor(p2Def.accentColor);
        p2Name.setOutlineColor(sf::Color::Black);
        p2Name.setOutlineThickness(3.0f);
        p2Name.setOrigin(sf::Vector2f(p2Name.getLocalBounds().size.x * 0.5f, 0.0f));
        p2Name.setPosition(sf::Vector2f(1220.0f, 540.0f));
        window.draw(p2Name);

        sf::Text p2Title(*m_font, p2Def.title, 14);
        p2Title.setStyle(sf::Text::Bold);
        p2Title.setFillColor(sf::Color(235, 180, 195));
        p2Title.setOrigin(sf::Vector2f(p2Title.getLocalBounds().size.x * 0.5f, 0.0f));
        p2Title.setPosition(sf::Vector2f(1220.0f, 585.0f));
        window.draw(p2Title);

        // Center "VS" Emblem
        sf::CircleShape vsCircle(75.0f);
        vsCircle.setOrigin(sf::Vector2f(75.0f, 75.0f));
        vsCircle.setPosition(sf::Vector2f(800.0f, 450.0f));
        vsCircle.setFillColor(sf::Color(18, 20, 28));
        vsCircle.setOutlineColor(sf::Color(255, 215, 60));
        vsCircle.setOutlineThickness(4.0f);
        window.draw(vsCircle);

        sf::Text vsText(*m_font, "VS", 64);
        vsText.setStyle(sf::Text::Bold);
        vsText.setFillColor(sf::Color(255, 215, 60));
        vsText.setOutlineColor(sf::Color::Black);
        vsText.setOutlineThickness(4.0f);
        vsText.setOrigin(sf::Vector2f(vsText.getLocalBounds().size.x * 0.5f, vsText.getLocalBounds().size.y * 0.5f + 8.0f));
        vsText.setPosition(sf::Vector2f(800.0f, 450.0f));
        window.draw(vsText);

        // Bottom Stage Banner
        if (m_stageRenderer) {
            sf::Text stg(*m_font, "ARENA: " + m_stageRenderer->getStageName() + " // " + m_stageRenderer->getStageSubtitle(), 14);
            stg.setStyle(sf::Text::Bold);
            stg.setFillColor(m_stageRenderer->getStageThemeColor());
            stg.setOrigin(sf::Vector2f(stg.getLocalBounds().size.x * 0.5f, 0.0f));
            stg.setPosition(sf::Vector2f(800.0f, 820.0f));
            window.draw(stg);
        }
    }

    // -------------------------------------------------------------------------
    // 7. IN-GAME PAUSE MENU OVERLAY
    // -------------------------------------------------------------------------
    void drawPauseMenu(sf::RenderWindow& window) {
        sf::RectangleShape mask(sf::Vector2f(1600.0f, 900.0f));
        mask.setFillColor(sf::Color(8, 10, 16, 215));
        window.draw(mask);

        // Header Title
        sf::Text title(*m_font, "PAUSED", 48);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color::White);
        title.setOutlineColor(sf::Color(45, 145, 255));
        title.setOutlineThickness(3.0f);
        title.setOrigin(sf::Vector2f(title.getLocalBounds().size.x * 0.5f, 0.0f));
        title.setPosition(sf::Vector2f(800.0f, 180.0f));
        window.draw(title);

        const std::vector<std::string> pauseOpts = {
            "RESUME MATCH",
            "COMMAND LIST & MOVES",
            "RESTART MATCH",
            "CHARACTER SELECT",
            "MAIN MENU"
        };

        for (size_t i = 0; i < pauseOpts.size(); ++i) {
            bool isSelected = (static_cast<int>(i) == m_pauseSelectedIdx);
            float btnY = 290.0f + i * 62.0f;

            sf::RectangleShape btn(sf::Vector2f(400.0f, 48.0f));
            btn.setOrigin(sf::Vector2f(200.0f, 24.0f));
            btn.setPosition(sf::Vector2f(800.0f, btnY + 24.0f));

            if (isSelected) {
                btn.setFillColor(sf::Color(35, 50, 75, 240));
                btn.setOutlineColor(sf::Color(255, 215, 60));
                btn.setOutlineThickness(2.8f);
            } else {
                btn.setFillColor(sf::Color(18, 22, 32, 200));
                btn.setOutlineColor(sf::Color(55, 65, 85));
                btn.setOutlineThickness(1.5f);
            }
            window.draw(btn);

            sf::Text optText(*m_font, pauseOpts[i], 16);
            optText.setStyle(sf::Text::Bold);
            optText.setFillColor(isSelected ? sf::Color::White : sf::Color(190, 205, 225));
            optText.setOrigin(sf::Vector2f(optText.getLocalBounds().size.x * 0.5f, optText.getLocalBounds().size.y * 0.5f + 3.0f));
            optText.setPosition(sf::Vector2f(800.0f, btnY + 24.0f));
            window.draw(optText);

            if (isSelected) {
                sf::Text arrow(*m_font, ">", 20);
                arrow.setStyle(sf::Text::Bold);
                arrow.setFillColor(sf::Color(255, 215, 60));
                arrow.setPosition(sf::Vector2f(615.0f, btnY + 11.0f));
                window.draw(arrow);
            }
        }
    }

private:
    const sf::Font* m_font{ nullptr };
    RagdollEngine::RagdollRenderer* m_ragdollRenderer{ nullptr };
    RagdollEngine::StageRenderer* m_stageRenderer{ nullptr };

    MenuState m_state{ MenuState::TitleScreen };
    MenuState m_prevModalState{ MenuState::TitleScreen };
    GameMode m_gameMode{ GameMode::Versus };

    size_t m_p1CharIdx{ 0 };
    size_t m_p2CharIdx{ 1 };
    bool m_p1Ready{ false };
    bool m_p2Ready{ false };

    int m_stageIdx{ 0 };
    int m_titleSelectedIdx{ 0 };
    int m_pauseSelectedIdx{ 0 };

    float m_animTime{ 0.0f };
    float m_versusTimer{ 2.2f };
    float m_lockinTimer{ 0.0f };

    sf::Vector2f m_mousePos{ 0.0f, 0.0f };

    bool m_startMatchRequested{ false };
    bool m_restartMatchRequested{ false };
    bool m_quitRequested{ false };

    NetworkManager* m_netManager{ nullptr };
    std::string m_joinIpInput{ "127.0.0.1" };
    bool m_ipInputFocused{ false };
    bool m_lobbyStateDirty{ false };
    float m_lobbyConnectedTimer{ 0.0f };
};

} // namespace StickminGame
