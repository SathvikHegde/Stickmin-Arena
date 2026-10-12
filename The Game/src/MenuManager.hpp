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
#include "CpuController.hpp"

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace StickminGame {

enum class MenuState {
    TitleScreen,
    OnlineLobbyModal,
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
    VsCpu
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
            m_p2Ready = (m_gameMode == GameMode::Practice || m_gameMode == GameMode::VsCpu);
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

    CpuDifficulty getCpuDifficulty() const { return m_cpuDifficulty; }
    void setCpuDifficulty(CpuDifficulty diff) { m_cpuDifficulty = diff; }
    void cycleCpuDifficulty() {
        if (m_cpuDifficulty == CpuDifficulty::Easy) m_cpuDifficulty = CpuDifficulty::Medium;
        else if (m_cpuDifficulty == CpuDifficulty::Medium) m_cpuDifficulty = CpuDifficulty::Hard;
        else m_cpuDifficulty = CpuDifficulty::Easy;
    }

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
        // 2. Command List Modal
        else if (m_state == MenuState::CommandListModal) {
            if (key == sf::Keyboard::Key::A || key == sf::Keyboard::Key::Left) {
                m_cmdListTab = (m_cmdListTab + 3) % 4;
            } else if (key == sf::Keyboard::Key::D || key == sf::Keyboard::Key::Right || key == sf::Keyboard::Key::Tab) {
                m_cmdListTab = (m_cmdListTab + 1) % 4;
            } else if (key == sf::Keyboard::Key::Num1) {
                m_cmdListTab = 0;
            } else if (key == sf::Keyboard::Key::Num2) {
                m_cmdListTab = 1;
            } else if (key == sf::Keyboard::Key::Num3) {
                m_cmdListTab = 2;
            } else if (key == sf::Keyboard::Key::Num4) {
                m_cmdListTab = 3;
            } else if (key == sf::Keyboard::Key::Escape || key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::F3) {
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
            } else if (m_gameMode == GameMode::VsCpu) {
                // In VS CPU:
                // Player 1: A / D to navigate character, J / Space / Enter to toggle ready
                if (!m_p1Ready) {
                    if (key == sf::Keyboard::Key::A) {
                        m_p1CharIdx = (m_p1CharIdx + roster.size() - 1) % roster.size();
                    } else if (key == sf::Keyboard::Key::D) {
                        m_p1CharIdx = (m_p1CharIdx + 1) % roster.size();
                    }
                }
                if (key == sf::Keyboard::Key::J || key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::Enter) {
                    m_p1Ready = !m_p1Ready;
                    m_p2Ready = true; // CPU opponent is always ready
                }

                // Left / Right arrow keys change CPU opponent character!
                if (key == sf::Keyboard::Key::Left) {
                    m_p2CharIdx = (m_p2CharIdx + roster.size() - 1) % roster.size();
                } else if (key == sf::Keyboard::Key::Right) {
                    m_p2CharIdx = (m_p2CharIdx + 1) % roster.size();
                }

                // Tab key cycles CPU difficulty
                if (key == sf::Keyboard::Key::Tab) {
                    cycleCpuDifficulty();
                }

                if (key == sf::Keyboard::Key::Escape) {
                    if (m_p1Ready) {
                        m_p1Ready = false;
                    } else {
                        m_state = MenuState::TitleScreen;
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
        // 2. Command List Modal Click
        else if (m_state == MenuState::CommandListModal && button == sf::Mouse::Button::Left) {
            // Check tab clicks
            const float tabW = 285.0f;
            const float tabH = 34.0f;
            const float tabSpacing = 12.0f;
            const float tabStartY = 125.0f;
            const float tabStartX = 210.0f;
            for (int i = 0; i < 4; ++i) {
                sf::FloatRect tabRect(sf::Vector2f(tabStartX + i * (tabW + tabSpacing), tabStartY), sf::Vector2f(tabW, tabH));
                if (tabRect.contains(mousePos)) {
                    m_cmdListTab = i;
                    return true;
                }
            }

            // Close button (bottom center)
            sf::FloatRect closeBtn(sf::Vector2f(690.0f, 776.0f), sf::Vector2f(220.0f, 38.0f));
            // Close 'X' button (top-right)
            sf::FloatRect closeX(sf::Vector2f(1374.0f, 81.0f), sf::Vector2f(30.0f, 30.0f));
            // Modal box bounds
            sf::FloatRect modalBox(sf::Vector2f(180.0f, 65.0f), sf::Vector2f(1240.0f, 770.0f));

            if (closeBtn.contains(mousePos) || closeX.contains(mousePos) || !modalBox.contains(mousePos)) {
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

            // VS CPU Mode clicks
            if (m_gameMode == GameMode::VsCpu) {
                sf::FloatRect p1Btn(sf::Vector2f(60.0f, 670.0f), sf::Vector2f(330.0f, 80.0f));
                if (p1Btn.contains(mousePos) && button == sf::Mouse::Button::Left) {
                    m_p1Ready = !m_p1Ready;
                    m_p2Ready = true;
                    return true;
                }

                // Clicking CPU button cycles difficulty!
                sf::FloatRect p2Btn(sf::Vector2f(1210.0f, 670.0f), sf::Vector2f(330.0f, 80.0f));
                if (p2Btn.contains(mousePos) && button == sf::Mouse::Button::Left) {
                    cycleCpuDifficulty();
                    return true;
                }

                // Character Cards Click in VS CPU
                float startX = 415.0f;
                float cardW = 145.0f;
                float cardGap = 16.0f;
                for (size_t i = 0; i < roster.size(); ++i) {
                    float cx = startX + i * (cardW + cardGap);
                    sf::FloatRect cardRect(sf::Vector2f(cx, 250.0f), sf::Vector2f(cardW, 200.0f));
                    if (cardRect.contains(mousePos)) {
                        if (button == sf::Mouse::Button::Right) {
                            m_p2CharIdx = i; // Right click picks CPU
                        } else if (button == sf::Mouse::Button::Left) {
                            if (!m_p1Ready) {
                                m_p1CharIdx = i; // Left click picks Player 1
                            } else {
                                m_p2CharIdx = i; // If P1 locked in, picks CPU
                            }
                        }
                        return true;
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
            case 2: // VS CPU
                m_gameMode = GameMode::VsCpu;
                m_p1Ready = false;
                m_p2Ready = true; // CPU opponent is auto-ready!
                m_state = MenuState::CharacterSelect;
                break;
            case 3: // PRACTICE / TRAINING
                m_gameMode = GameMode::Practice;
                m_p1Ready = false;
                m_p2Ready = true; // Practice: P2 is dummy, already ready!
                m_state = MenuState::CharacterSelect;
                break;
            case 4: // COMMAND LIST
                m_prevModalState = MenuState::TitleScreen;
                m_cmdListTab = 0;
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
                m_cmdListTab = 0;
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
                    m_p2Ready = (m_gameMode == GameMode::Practice || m_gameMode == GameMode::VsCpu);
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

        // Version badge
        sf::Text ver(*m_font, "Version: 0.67", 12);
        ver.setFillColor(sf::Color(130, 145, 170));
        ver.setOrigin(sf::Vector2f(ver.getLocalBounds().size.x * 0.5f, 0.0f));
        ver.setPosition(sf::Vector2f(800.0f, 195.0f));
        window.draw(ver);

        // Menu Options
        const std::vector<std::string> options = {
            "VERSUS (LOCAL)",
            "ONLINE MULTIPLAYER",
            "VS CPU",
            "PRACTICE",
            "COMMAND LIST",
            "EXIT"
        };

        for (size_t i = 0; i < options.size(); ++i) {
            bool isSelected = (static_cast<int>(i) == m_titleSelectedIdx);
            float btnY = 280.0f + i * 54.0f;

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
        sf::Text hint(*m_font, "[W / S] Navigate   |   [ENTER / SPACE] Select   |   [ESC] Quit", 13);
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

        sf::Text sub(*m_font, "Host or Join via IP", 13);
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

        sf::Text backText(*m_font, "< Back to Menu", 14);
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

            sf::Text procText(*m_font, "Character Select  >>", 17);
            procText.setStyle(sf::Text::Bold);
            procText.setFillColor(sf::Color::White);
            procText.setOrigin(sf::Vector2f(procText.getLocalBounds().size.x * 0.5f, procText.getLocalBounds().size.y * 0.5f + 2.0f));
            procText.setPosition(sf::Vector2f(800.0f, 682.0f));
            window.draw(procText);
        }
    }

    // -------------------------------------------------------------------------
    // 2. COMMAND LIST MODAL (TABBED TEKKEN 8 STYLE SYSTEM GUIDE)
    // -------------------------------------------------------------------------
    void drawCommandListModal(sf::RenderWindow& window) {
        if (!m_font) return;

        // Dark dimming overlay mask
        sf::RectangleShape mask(sf::Vector2f(1600.0f, 900.0f));
        mask.setFillColor(sf::Color(6, 8, 14, 240));
        window.draw(mask);

        // Modal dimensions: 1240 x 770, Top-Left at (180, 65)
        const float modalX = 180.0f;
        const float modalY = 65.0f;
        const float modalW = 1240.0f;
        const float modalH = 770.0f;

        // Outer Modal Box
        sf::RectangleShape box(sf::Vector2f(modalW, modalH));
        box.setPosition(sf::Vector2f(modalX, modalY));
        box.setFillColor(sf::Color(14, 18, 26, 252));
        box.setOutlineColor(sf::Color(45, 130, 240));
        box.setOutlineThickness(2.5f);
        window.draw(box);

        // Top Accent Gold Line
        sf::RectangleShape accentLine(sf::Vector2f(modalW, 3.0f));
        accentLine.setPosition(sf::Vector2f(modalX, modalY));
        accentLine.setFillColor(sf::Color(255, 215, 60));
        window.draw(accentLine);

        // Header Title
        sf::Text title(*m_font, "COMMAND LIST & SYSTEM MECHANICS", 22);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color::White);
        title.setPosition(sf::Vector2f(modalX + 28.0f, modalY + 14.0f));
        window.draw(title);

        sf::Text subTitle(*m_font, "Tekken-Style Ragdoll Combat System  *  Navigate with [A] / [D] / [TAB] or Click Tabs", 12);
        subTitle.setFillColor(sf::Color(255, 215, 70));
        subTitle.setPosition(sf::Vector2f(modalX + 30.0f, modalY + 42.0f));
        window.draw(subTitle);

        // Close 'X' Button at top-right
        sf::FloatRect closeXRect(sf::Vector2f(1374.0f, 81.0f), sf::Vector2f(30.0f, 30.0f));
        bool closeXHover = closeXRect.contains(m_mousePos);
        sf::RectangleShape closeXBox(sf::Vector2f(30.0f, 30.0f));
        closeXBox.setPosition(sf::Vector2f(1374.0f, 81.0f));
        closeXBox.setFillColor(closeXHover ? sf::Color(180, 40, 40, 220) : sf::Color(35, 42, 58, 180));
        closeXBox.setOutlineColor(closeXHover ? sf::Color(255, 100, 100) : sf::Color(70, 85, 110));
        closeXBox.setOutlineThickness(1.5f);
        window.draw(closeXBox);

        sf::Text closeXText(*m_font, "X", 14);
        closeXText.setStyle(sf::Text::Bold);
        closeXText.setFillColor(closeXHover ? sf::Color::White : sf::Color(190, 205, 225));
        closeXText.setOrigin(sf::Vector2f(closeXText.getLocalBounds().size.x * 0.5f, closeXText.getLocalBounds().size.y * 0.5f + 2.0f));
        closeXText.setPosition(sf::Vector2f(1389.0f, 96.0f));
        window.draw(closeXText);

        // ---------------------------------------------------------------------
        // TAB BAR (Y: 125 to 159)
        // ---------------------------------------------------------------------
        const std::vector<std::string> tabTitles = {
            "1. MOVES & FRAME DATA",
            "2. OFFENSE & COMBOS",
            "3. DEFENSE & SYSTEM",
            "4. CONTROLS & SHORTCUTS"
        };

        const float tabW = 285.0f;
        const float tabH = 34.0f;
        const float tabSpacing = 12.0f;
        const float tabStartX = 210.0f;
        const float tabStartY = 125.0f;

        for (size_t i = 0; i < tabTitles.size(); ++i) {
            float tx = tabStartX + i * (tabW + tabSpacing);
            sf::FloatRect tRect(sf::Vector2f(tx, tabStartY), sf::Vector2f(tabW, tabH));
            bool isCurrent = (static_cast<int>(i) == m_cmdListTab);
            bool isHovered = tRect.contains(m_mousePos);

            sf::RectangleShape tabBtn(sf::Vector2f(tabW, tabH));
            tabBtn.setPosition(sf::Vector2f(tx, tabStartY));

            if (isCurrent) {
                tabBtn.setFillColor(sf::Color(30, 75, 145, 240));
                tabBtn.setOutlineColor(sf::Color(255, 215, 60));
                tabBtn.setOutlineThickness(2.2f);
            } else if (isHovered) {
                tabBtn.setFillColor(sf::Color(35, 48, 70, 220));
                tabBtn.setOutlineColor(sf::Color(80, 160, 255));
                tabBtn.setOutlineThickness(1.5f);
            } else {
                tabBtn.setFillColor(sf::Color(22, 28, 40, 200));
                tabBtn.setOutlineColor(sf::Color(55, 70, 95));
                tabBtn.setOutlineThickness(1.2f);
            }
            window.draw(tabBtn);

            // Active underline indicator
            if (isCurrent) {
                sf::RectangleShape activeLine(sf::Vector2f(tabW, 3.0f));
                activeLine.setPosition(sf::Vector2f(tx, tabStartY + tabH - 3.0f));
                activeLine.setFillColor(sf::Color(255, 215, 60));
                window.draw(activeLine);
            }

            sf::Text tabTxt(*m_font, tabTitles[i], 13);
            tabTxt.setStyle(sf::Text::Bold);
            tabTxt.setFillColor(isCurrent ? sf::Color(255, 255, 255) : (isHovered ? sf::Color(220, 235, 255) : sf::Color(170, 185, 205)));
            tabTxt.setOrigin(sf::Vector2f(tabTxt.getLocalBounds().size.x * 0.5f, tabTxt.getLocalBounds().size.y * 0.5f + 3.0f));
            tabTxt.setPosition(sf::Vector2f(tx + tabW * 0.5f, tabStartY + tabH * 0.5f));
            window.draw(tabTxt);
        }

        // Shared drawing lambdas
        auto drawCard = [&](float x, float y, float w, float h, sf::Color fill, sf::Color border = sf::Color(55, 70, 95), float thick = 1.2f) {
            sf::RectangleShape card(sf::Vector2f(w, h));
            card.setPosition(sf::Vector2f(x, y));
            card.setFillColor(fill);
            card.setOutlineColor(border);
            card.setOutlineThickness(thick);
            window.draw(card);
        };

        auto drawPill = [&](float x, float y, float w, float h, const std::string& text, sf::Color fill, sf::Color textColor, sf::Color outline = sf::Color::Transparent) {
            sf::RectangleShape pill(sf::Vector2f(w, h));
            pill.setPosition(sf::Vector2f(x, y));
            pill.setFillColor(fill);
            if (outline != sf::Color::Transparent) {
                pill.setOutlineColor(outline);
                pill.setOutlineThickness(1.0f);
            }
            window.draw(pill);

            sf::Text pt(*m_font, text, 10);
            pt.setStyle(sf::Text::Bold);
            pt.setFillColor(textColor);
            pt.setOrigin(sf::Vector2f(pt.getLocalBounds().size.x * 0.5f, pt.getLocalBounds().size.y * 0.5f + 2.0f));
            pt.setPosition(sf::Vector2f(x + w * 0.5f, y + h * 0.5f));
            window.draw(pt);
        };

        auto drawSectionHeader = [&](float x, float y, float w, const std::string& hdrTitle, sf::Color barCol, sf::Color textCol = sf::Color::White) {
            sf::RectangleShape bar(sf::Vector2f(w, 28.0f));
            bar.setPosition(sf::Vector2f(x, y));
            bar.setFillColor(barCol);
            window.draw(bar);

            sf::Text st(*m_font, hdrTitle, 13);
            st.setStyle(sf::Text::Bold);
            st.setFillColor(textCol);
            st.setPosition(sf::Vector2f(x + 12.0f, y + 5.0f));
            window.draw(st);
        };

        // =====================================================================
        // TAB 0: MOVES & FRAME DATA
        // =====================================================================
        if (m_cmdListTab == 0) {
            struct MoveCardData {
                std::string inputP1;
                std::string inputP2;
                std::string notation;
                std::string name;
                std::string height;
                sf::Color heightCol;
                std::string damage;
                std::string frameData;
                std::string notes;
                std::string tag;
                sf::Color tagCol;
            };

            const std::vector<MoveCardData> moves = {
                // Left Column (0-5)
                { "[J]", "[Num1]", "1 (LP)", "Flash Jab", "HIGH", sf::Color(240, 185, 50), "9.0", "Startup: 0.06s (i3.6) | Act: 0.05s | Rec: 0.08s", "Fastest standing poke in the game. Plus frames on hit; checks incoming rushes.", "", sf::Color::Transparent },
                { "[K]", "[Num2]", "2 (RP)", "Straight Cross", "MID", sf::Color(65, 190, 255), "15.0", "Startup: 0.08s (i4.8) | Act: 0.06s | Rec: 0.10s", "Mid check; cannot be ducked. Stops crouching opponents and pushes back.", "", sf::Color::Transparent },
                { "[J]->[K]", "[Num1]->[2]", "1, 2", "1, 2 String", "HIGH", sf::Color(240, 185, 50), "17.0", "Startup: 0.07s (i4.2) | Act: 0.09s | Rec: 0.11s", "Natural two-hit jab combo. Staggers target; maintains frame dominance.", "NATURAL", sf::Color(100, 220, 120) },
                { "[U]", "[Num4]", "3 (LK)", "Mid Kick", "MID", sf::Color(65, 190, 255), "14.0", "Startup: 0.09s (i5.4) | Act: 0.06s | Rec: 0.11s", "Mid spacing kick. Great mid-range check to stuff approaching moves.", "", sf::Color::Transparent },
                { "[S+U]", "[Down+4]", "d+3", "Hell Sweep", "LOW", sf::Color(255, 90, 60), "16.0", "Startup: 0.11s (i6.6) | Act: 0.07s | Rec: 0.13s", "Low trip sweep. Pierces high guard; knocks down opponent into tumble.", "TRIP", sf::Color(255, 120, 50) },
                { "[I]", "[Num5]", "4 (RK)", "Axe Roundhouse", "HIGH", sf::Color(240, 185, 50), "22.0", "Startup: 0.12s (i7.2) | Act: 0.07s | Rec: 0.15s", "Heavy roundhouse (15x, -7.5y). Counter-Hit Launcher for full air juggles!", "CH LAUNCH", sf::Color(255, 75, 75) },

                // Right Column (6-11)
                { "[D+K]", "[Right+2]", "f+2", "Electric Wind God Fist", "MID", sf::Color(65, 190, 255), "25.0", "Startup: 0.12s (i7.2) | Act: 0.08s | Rec: 0.14s", "Premier launcher (4x, -17.5y)! Electric sparks; initiates max air juggles.", "ELECTRIC", sf::Color(255, 225, 40) },
                { "[W+I]", "[Up+5]", "u+4", "Hopkick", "MID", sf::Color(65, 190, 255), "18.0", "Startup: 0.10s (i6.0) | Act: 0.07s | Rec: 0.16s", "Universal mid launcher (4.5x, -15.5y). Low Crush: jumps over low sweeps!", "LAUNCHER", sf::Color(100, 220, 255) },
                { "[P]", "[Num9]", "3+4", "Flying Dropkick", "MID", sf::Color(65, 190, 255), "28.0", "Startup: 0.14s (i8.4) | Act: 0.12s | Rec: 0.20s", "Supersonic dive (20x, -6.5y). Full screen reach; massive Wall Splat tool!", "WALL SPLAT", sf::Color(255, 160, 30) },
                { "[O]", "[Num3]", "1+2", "Power Crush", "MID", sf::Color(65, 190, 255), "24.0", "Startup: 0.15s (i9.0) | Act: 0.08s | Rec: 0.17s", "Hyper Armor: Absorbs High/Mid hits (50% dmg, 0 hitstun). Loses to lows.", "ARMOR", sf::Color(255, 140, 40) },
                { "[L]", "[Num6]", "1+3", "Command Throw", "HIGH", sf::Color(240, 185, 50), "26.0", "Startup: 0.12s (i7.2) | Act: 0.08s | Rec: 0.16s", "Unblockable 34px reach grab! Ignores standing guard; duck or attack to break.", "UNBLOCK", sf::Color(200, 110, 255) },
                { "[O] in Rage", "[Num3] Rage", "1+2 Rage", "Rage Art Finisher", "MID", sf::Color(65, 190, 255), "55.2", "Startup: 0.18s (i10.8) | Act: 0.12s | Rec: 0.22s", "Super armored launcher (24x, -14y). +15% Rage boost, slow-mo cinematic!", "SUPER", sf::Color(255, 45, 65) }
            };

            const float colW = 580.0f;
            const float cardH = 85.0f;
            const float cardGap = 7.0f;
            const float startY = 172.0f;

            for (size_t i = 0; i < moves.size(); ++i) {
                float cx = (i < 6) ? 210.0f : 810.0f;
                float cy = startY + (i % 6) * (cardH + cardGap);
                const auto& m = moves[i];

                drawCard(cx, cy, colW, cardH, sf::Color(20, 25, 36, 230), sf::Color(45, 58, 80));

                // Input Pill (P1 / P2)
                drawPill(cx + 8.0f, cy + 9.0f, 62.0f, 20.0f, m.notation, sf::Color(32, 45, 68), sf::Color(255, 220, 75), sf::Color(65, 90, 130));

                // Move Name
                sf::Text nameTxt(*m_font, m.name, 14);
                nameTxt.setStyle(sf::Text::Bold);
                nameTxt.setFillColor(sf::Color::White);
                nameTxt.setPosition(sf::Vector2f(cx + 78.0f, cy + 9.0f));
                window.draw(nameTxt);

                // Attack Height Badge
                drawPill(cx + 310.0f, cy + 9.0f, 44.0f, 20.0f, m.height, m.heightCol, sf::Color(15, 20, 30));

                // Tag badge if present
                if (!m.tag.empty()) {
                    drawPill(cx + 360.0f, cy + 9.0f, 68.0f, 20.0f, m.tag, m.tagCol, sf::Color(15, 20, 30));
                }

                // Damage Badge
                drawPill(cx + 495.0f, cy + 9.0f, 75.0f, 20.0f, "DMG: " + m.damage, sf::Color(25, 42, 32), sf::Color(110, 240, 150), sf::Color(50, 100, 70));

                // Frame data line
                sf::Text fdTxt(*m_font, m.frameData, 11);
                fdTxt.setFillColor(sf::Color(180, 195, 215));
                fdTxt.setPosition(sf::Vector2f(cx + 10.0f, cy + 36.0f));
                window.draw(fdTxt);

                // Keys mapping
                sf::Text keysTxt(*m_font, "P1: " + m.inputP1 + "  |  P2: " + m.inputP2, 10);
                keysTxt.setFillColor(sf::Color(135, 150, 175));
                keysTxt.setPosition(sf::Vector2f(cx + 375.0f, cy + 36.0f));
                window.draw(keysTxt);

                // Tactical note line
                sf::Text noteTxt(*m_font, m.notes, 11);
                noteTxt.setFillColor(sf::Color(215, 225, 240));
                noteTxt.setPosition(sf::Vector2f(cx + 10.0f, cy + 58.0f));
                window.draw(noteTxt);
            }
        }
        // =====================================================================
        // TAB 1: OFFENSE & COMBOS
        // =====================================================================
        else if (m_cmdListTab == 1) {
            const float pW = 580.0f;
            const float pH = 268.0f;

            // Panel 1: Attack Height Matrix (Top-Left)
            drawCard(210.0f, 172.0f, pW, pH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(210.0f, 172.0f, pW, "1. ATTACK HEIGHT ROCK-PAPER-SCISSORS", sf::Color(32, 55, 90));

            sf::Text hMatrix(*m_font,
                "HIGH ATTACKS (Flash Jab, 1,2 String, Axe Roundhouse, Throw):\n"
                "  * Fastest startup (3-7 frames) with frame advantage on block.\n"
                "  * VULNERABILITY: Duckable! Crouching/low-guarding foes evade highs completely.\n\n"
                "MID ATTACKS (Straight Cross, EWGF, Hopkick, Dropkick, Power Crush):\n"
                "  * Core offensive weapon: Hits both standing and crouching opponents!\n"
                "  * CRUSHES LOW GUARD: Punishes ducking defenders for full clean hit damage.\n\n"
                "LOW ATTACKS (Hell Sweep [d+3]):\n"
                "  * Pierces High & Standing Neutral Guards completely, tripping into tumble!\n"
                "  * VULNERABILITY: Blocked by Low Guard (Down+Back); slow on recovery.", 11);
            hMatrix.setFillColor(sf::Color(215, 225, 240));
            hMatrix.setPosition(sf::Vector2f(222.0f, 210.0f));
            window.draw(hMatrix);

            // Panel 2: Counter Hit & Punish System (Top-Right)
            drawCard(810.0f, 172.0f, pW, pH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(810.0f, 172.0f, pW, "2. COUNTER HIT & PUNISH MECHANICS", sf::Color(90, 35, 45));

            sf::Text chMatrix(*m_font,
                "COUNTER HIT TRIGGER & DAMAGE SURGE:\n"
                "  * Striking an opponent while they are in move startup triggers COUNTER HIT!\n"
                "  * +45% DAMAGE BOOST: Deals 1.45x damage on the connecting strike.\n"
                "  * CINEMATIC IMPACT: Freezes game for 0.13s Hitstop + 0.12x Slow-Mo zoom!\n\n"
                "COUNTER LAUNCHERS:\n"
                "  * Heavy attacks like Axe Roundhouse [4] become full vertical Launchers\n"
                "    specifically on Counter Hit, opening up extended air juggles!\n\n"
                "PUNISH ADVANTAGE:\n"
                "  * Striking an opponent during their move recovery triggers PUNISH alert.\n"
                "  * Guaranteed damage: Defender cannot block, tech, or escape recovery frames.", 11);
            chMatrix.setFillColor(sf::Color(215, 225, 240));
            chMatrix.setPosition(sf::Vector2f(822.0f, 210.0f));
            window.draw(chMatrix);

            // Panel 3: Air Juggle & Scaling (Bottom-Left)
            drawCard(210.0f, 450.0f, pW, pH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(210.0f, 450.0f, pW, "3. AIR JUGGLE PHYSICS & ZERO DAMAGE SCALING", sf::Color(35, 75, 70));

            sf::Text jglMatrix(*m_font,
                "UNIVERSAL LAUNCHERS:\n"
                "  * EWGF [f+2] (4x, -17.5y), Hopkick [u+4] (4.5x, -15.5y), CH Roundhouse [4].\n"
                "  * Puts victim into LaunchedJuggle; joint springs relax to 6.0 Hz float.\n\n"
                "JUGGLE POP-UP IMPULSE:\n"
                "  * Every hit during LaunchedJuggle applies pop-up impulse (facing*2.5, -9.5)\n"
                "    to BOTH Hips & Torso (-19.0 total lift), suspending victim for combos!\n\n"
                "ZERO DAMAGE SCALING EXPLOIT:\n"
                "  * The combat engine currently has NO damage scaling on juggle hits!\n"
                "  * Every juggle hit inflicts 100% full unscaled damage.\n"
                "  * Example: EWGF (25) -> Jab (9) -> 1,2 (17) -> Dropkick (28) = 79 DMG (80% KO!)", 11);
            jglMatrix.setFillColor(sf::Color(215, 225, 240));
            jglMatrix.setPosition(sf::Vector2f(222.0f, 488.0f));
            window.draw(jglMatrix);

            // Panel 4: Wall Splats & Clashes (Bottom-Right)
            drawCard(810.0f, 450.0f, pW, pH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(810.0f, 450.0f, pW, "4. WALL SPLATS & SIMULTANEOUS STRIKE CLASHES", sf::Color(85, 65, 25));

            sf::Text wallMatrix(*m_font,
                "WALL SPLAT DYNAMICS:\n"
                "  * Arena Wall Zones: Triggered at Left Wall (x < 190) or Right Wall (x > 1410).\n"
                "  * Hitting opponent with heavy knockback (Dropkick, Power Crush, Launchers,\n"
                "    or Counter Hits) into the wall triggers a devastating WALL SPLAT!\n"
                "  * REWARDS: +8.0 Bonus Damage, camera trauma (0.65), wall impact cracks,\n"
                "    and Wall Stick (zeroes horizontal velocity) for guaranteed follow-up hits!\n\n"
                "STRIKE CLASH SYSTEM:\n"
                "  * When both fighters strike simultaneously within 125px (progress < 50%),\n"
                "    a dramatic CLASH triggers with 3.5x camera zoom, 0.08s slow-mo & recoil!", 11);
            wallMatrix.setFillColor(sf::Color(215, 225, 240));
            wallMatrix.setPosition(sf::Vector2f(822.0f, 488.0f));
            window.draw(wallMatrix);
        }
        // =====================================================================
        // TAB 2: DEFENSE & SYSTEM
        // =====================================================================
        else if (m_cmdListTab == 2) {
            const float pW = 580.0f;
            const float pH = 268.0f;

            // Panel 1: Complete Guarding Matrix (Top-Left)
            drawCard(210.0f, 172.0f, pW, pH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(210.0f, 172.0f, pW, "1. COMPLETE GUARDING & DUCKING MATRIX", sf::Color(32, 55, 90));

            sf::Text gMatrix(*m_font,
                "NEUTRAL GUARD (AUTOGUARD):\n"
                "  * Release all movement keys to stand still in Neutral.\n"
                "  * Automatically blocks High and Mid strikes with ZERO chip damage!\n\n"
                "BACK-WALK GUARD:\n"
                "  * Hold [Back] (away from opponent) to retreat while blocking Highs & Mids.\n\n"
                "CROUCH DUCKING (EVASION):\n"
                "  * Hold [Down] or [Down+Back] to crouch.\n"
                "  * Completely ducks under High punches and Command Throws (Clean Miss!).\n\n"
                "LOW GUARD:\n"
                "  * Hold [Down+Back] to block Low sweeps (Hell Sweep). Warning: Weak to Mids!", 11);
            gMatrix.setFillColor(sf::Color(215, 225, 240));
            gMatrix.setPosition(sf::Vector2f(222.0f, 210.0f));
            window.draw(gMatrix);

            // Panel 2: Power Crush Hyper Armor (Top-Right)
            drawCard(810.0f, 172.0f, pW, pH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(810.0f, 172.0f, pW, "2. POWER CRUSH (HYPER ARMOR)", sf::Color(85, 45, 25));

            sf::Text pcMatrix(*m_font,
                "INPUT & PROPERTIES:\n"
                "  * Input: [1+2] (P1: [O] / P2: [Num3]). Startup: 0.15s, Damage: 24.0.\n\n"
                "HYPER ARMOR ABSORPTION:\n"
                "  * Absorbs incoming High and Mid strikes during move startup!\n"
                "  * Takes only 50% damage and suffers ZERO hitstun, punching through\n"
                "    opponent attack strings to blow them away with 18.0x launch impulse!\n\n"
                "ARMOR BREAKERS & COUNTERS:\n"
                "  * Low attacks (Hell Sweep [d+3]) pierce armor and trip the armored fighter!\n"
                "  * Command Throws [1+3] grab clean through Power Crush armor.", 11);
            pcMatrix.setFillColor(sf::Color(215, 225, 240));
            pcMatrix.setPosition(sf::Vector2f(822.0f, 210.0f));
            window.draw(pcMatrix);

            // Panel 3: Command Throws & Throw Breaks (Bottom-Left)
            drawCard(210.0f, 450.0f, pW, pH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(210.0f, 450.0f, pW, "3. COMMAND THROWS & THROW BREAKS", sf::Color(70, 35, 85));

            sf::Text thMatrix(*m_font,
                "COMMAND THROW PROPERTIES:\n"
                "  * Input: [1+3] (P1: [L] / P2: [Num6]). Extended 34px grab reach, deals 26.0 DMG.\n"
                "  * UNBLOCKABLE: Bypasses Standing Guard, Neutral Guard, and Power Crush!\n\n"
                "THROW BREAKING (DEFENSE):\n"
                "  * Press ANY attack button during opponent's throw startup to execute\n"
                "    a THROW BREAK! Breaks the grab and pushes both fighters safely apart.\n\n"
                "DUCKING EVASION:\n"
                "  * Crouching / Low Guarding completely evades throws (Miss!), granting\n"
                "    a massive whiff punish window to launch the recovering opponent!", 11);
            thMatrix.setFillColor(sf::Color(215, 225, 240));
            thMatrix.setPosition(sf::Vector2f(222.0f, 488.0f));
            window.draw(thMatrix);

            // Panel 4: Tech Roll & Rage Mode (Bottom-Right)
            drawCard(810.0f, 450.0f, pW, pH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(810.0f, 450.0f, pW, "4. GROUND TECH ROLLS & RAGE MODE", sf::Color(90, 30, 40));

            sf::Text rgMatrix(*m_font,
                "GROUND TECH ROLL (GETUP):\n"
                "  * When knocked down, tap any attack key or wait 0.45s to execute a roll getup.\n"
                "  * INVULNERABILITY: Grants 0.35s total invulnerability, escaping all ground traps!\n\n"
                "RAGE MODE (TEKKEN 7 / 8 AURA):\n"
                "  * Activates automatically when HP drops below 28% (HP <= 28).\n"
                "  * Emits pulsing red rage aura; grants +15% FLAT DAMAGE BOOST to all attacks!\n\n"
                "RAGE ART FINISHER:\n"
                "  * Press [1+2] while in Rage. Armored super launcher dealing 55.2 damage!\n"
                "  * Features cinematic camera zoom, screen flash, and 1.6s dramatic slow-mo.", 11);
            rgMatrix.setFillColor(sf::Color(215, 225, 240));
            rgMatrix.setPosition(sf::Vector2f(822.0f, 488.0f));
            window.draw(rgMatrix);
        }
        // =====================================================================
        // TAB 3: CONTROLS & SHORTCUTS
        // =====================================================================
        else if (m_cmdListTab == 3) {
            const float pW = 580.0f;
            const float topH = 335.0f;

            // Player 1 Card (Top-Left)
            drawCard(210.0f, 172.0f, pW, topH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(210.0f, 172.0f, pW, "PLAYER 1 CONTROLS (KEYBOARD WASD + J/K/U/I)", sf::Color(30, 65, 115));

            sf::Text p1Ctrl(*m_font,
                "MOVEMENT & DASHING:\n"
                "  * [W] Jump / Up    [A] Walk Left / Guard    [S] Crouch    [D] Walk Right\n"
                "  * Double-Tap [D]: Forward Dash  |  Double-Tap [A]: Back Dash\n\n"
                "FOUR-BUTTON TEKKEN LAYOUT:\n"
                "  * 1 (LP / Left Punch)  : [J] (Flash Jab)\n"
                "  * 2 (RP / Right Punch) : [K] (Straight Cross  |  D+K: EWGF Launcher)\n"
                "  * 3 (LK / Left Kick)   : [U] (Mid Kick        |  S+U: Hell Sweep)\n"
                "  * 4 (RK / Right Kick)  : [I] (Axe Roundhouse  |  W+I: Hopkick)\n\n"
                "NATURAL COMBO & FAST SHORTCUTS:\n"
                "  * [J] -> [K] : 1, 2 Natural Jab String\n"
                "  * [O] : 1+2 (Power Crush Armor  |  Rage Art when in Rage!)\n"
                "  * [L] : 1+3 (Command Throw - Unblockable Grab)\n"
                "  * [P] : 3+4 (Flying Dropkick - Full Screen Wall Splat)", 11);
            p1Ctrl.setFillColor(sf::Color(215, 225, 240));
            p1Ctrl.setPosition(sf::Vector2f(222.0f, 210.0f));
            window.draw(p1Ctrl);

            // Player 2 Card (Top-Right)
            drawCard(810.0f, 172.0f, pW, topH, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(810.0f, 172.0f, pW, "PLAYER 2 CONTROLS (ARROWS + NUMPAD / LOCAL 2P)", sf::Color(100, 40, 45));

            sf::Text p2Ctrl(*m_font,
                "MOVEMENT & DASHING:\n"
                "  * [Up] Jump    [Left] Walk Left / Guard    [Down] Crouch    [Right] Walk Right\n"
                "  * Double-Tap [Right]: Forward Dash  |  Double-Tap [Left]: Back Dash\n\n"
                "FOUR-BUTTON TEKKEN LAYOUT:\n"
                "  * 1 (LP) : [Numpad 1] or [,] (Flash Jab)\n"
                "  * 2 (RP) : [Numpad 2] or [.] (Straight Cross  |  Right+2: EWGF Launcher)\n"
                "  * 3 (LK) : [Numpad 4] or [/] (Mid Kick        |  Down+4: Hell Sweep)\n"
                "  * 4 (RK) : [Numpad 5] or [;] (Axe Roundhouse  |  Up+5: Hopkick)\n\n"
                "NATURAL COMBO & FAST SHORTCUTS:\n"
                "  * [Num1] -> [Num2] : 1, 2 Natural Jab String\n"
                "  * [Num3] or []] : 1+2 (Power Crush Armor  |  Rage Art in Rage)\n"
                "  * [Num6] or ['] : 1+3 (Command Throw)\n"
                "  * [Num9] or [[] : 3+4 (Flying Dropkick)", 11);
            p2Ctrl.setFillColor(sf::Color(215, 225, 240));
            p2Ctrl.setPosition(sf::Vector2f(822.0f, 210.0f));
            window.draw(p2Ctrl);

            // System Shortcuts Card (Bottom)
            drawCard(210.0f, 517.0f, 1180.0f, 201.0f, sf::Color(20, 25, 36, 230), sf::Color(45, 60, 85));
            drawSectionHeader(210.0f, 517.0f, 1180.0f, "SYSTEM HOTKEYS, PRACTICE MODE & GAMEPLAY SHORTCUTS", sf::Color(40, 50, 75));

            sf::Text sysCtrl(*m_font,
                "[F3] KEY           : Toggle this Command List & Mechanics Modal anytime (in Battle, Title, or Pause).\n"
                "[ESC] KEY          : Pause match during battle / Resume game / Return from modals.\n"
                "[R] KEY            : Instant Match & Position Reset (Available in Practice Mode).\n"
                "[TAB] KEY          : Toggle Slow-Motion Simulation Mode (Available in Offline Versus & Practice).\n"
                "[Ctrl + V]         : Paste Host IP Address directly from system clipboard in Online Lobby.\n"
                "[A] / [D] / [Tab]  : Switch between tabs in this Command List menu  |  [1] - [4]: Jump directly to tab.\n"
                "[Space] / [Enter]  : Confirm menu selection  |  [Space] also acts as alternate jump for Player 1.", 11);
            sysCtrl.setFillColor(sf::Color(220, 230, 245));
            sysCtrl.setPosition(sf::Vector2f(225.0f, 552.0f));
            window.draw(sysCtrl);
        }

        // ---------------------------------------------------------------------
        // BOTTOM FOOTER & CLOSE BUTTON
        // ---------------------------------------------------------------------
        sf::Text navHelp(*m_font, "[A] / [D] / [Tab] Switch Tabs    *    [1]-[4] Direct Tab    *    [Esc] / [F3] Close", 12);
        navHelp.setFillColor(sf::Color(150, 170, 200));
        navHelp.setPosition(sf::Vector2f(modalX + 30.0f, modalY + modalH - 32.0f));
        window.draw(navHelp);

        // Centered Close Button
        sf::FloatRect closeRect(sf::Vector2f(690.0f, 776.0f), sf::Vector2f(220.0f, 38.0f));
        bool closeHover = closeRect.contains(m_mousePos);
        sf::RectangleShape closeBtn(sf::Vector2f(220.0f, 38.0f));
        closeBtn.setPosition(sf::Vector2f(690.0f, 776.0f));
        closeBtn.setFillColor(closeHover ? sf::Color(45, 125, 220) : sf::Color(28, 65, 125));
        closeBtn.setOutlineColor(closeHover ? sf::Color(255, 225, 80) : sf::Color(80, 150, 240));
        closeBtn.setOutlineThickness(closeHover ? 2.5f : 1.5f);
        window.draw(closeBtn);

        sf::Text closeText(*m_font, "CLOSE (ESC / F3)", 13);
        closeText.setStyle(sf::Text::Bold);
        closeText.setFillColor(closeHover ? sf::Color(255, 240, 120) : sf::Color::White);
        closeText.setOrigin(sf::Vector2f(closeText.getLocalBounds().size.x * 0.5f, closeText.getLocalBounds().size.y * 0.5f + 2.0f));
        closeText.setPosition(sf::Vector2f(800.0f, 795.0f));
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
        if (m_gameMode == GameMode::VsCpu) {
            subStr = "VS CPU MODE // [A/D] PICK P1 | [LEFT/RIGHT] PICK CPU | [TAB] DIFFICULTY: " + std::string(getDifficultyName(m_cpuDifficulty)) + " | [J / SPACE] LOCK IN";
        } else if (m_gameMode == GameMode::Online && m_netManager) {
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
        std::string headerTag = (playerNum == 1) ? "PLAYER 1"
                              : (m_gameMode == GameMode::VsCpu) ? ("CPU OPPONENT [" + std::string(getDifficultyName(m_cpuDifficulty)) + "]")
                              : "PLAYER 2";
        sf::Text tag(*m_font, headerTag, 14);
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
        if (charIdx == 0) moveDesc = "Balanced all-round fighting style.\nFeatures electric launcher and low sweeps.";
        else if (charIdx == 1) moveDesc = "Agile striker with fast pokes.\nStrong aerial juggles and hopkick.";
        else if (charIdx == 2) moveDesc = "High momentum and forward carry.\nFeatures flying dropkicks.";
        else if (charIdx == 3) moveDesc = "Defensive counter-hitter.\nEnhanced armored Power Crush.";
        else if (charIdx == 4) moveDesc = "Heavy cyborg bruiser.\nHigh damage output with armor properties.";

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

        std::string statusStr = isReady
            ? ((m_gameMode == GameMode::VsCpu && playerNum == 2)
                ? ("CPU: " + std::string(getDifficultyName(m_cpuDifficulty)) + " [READY]")
                : "READY [LOCKED IN]")
            : (playerNum == 1 ? "P1: PRESS [J] TO LOCK"
               : (m_gameMode == GameMode::Online ? "P2: PRESS [J] TO LOCK"
                  : (m_gameMode == GameMode::VsCpu ? ("CPU: " + std::string(getDifficultyName(m_cpuDifficulty)) + " [CLICK/TAB TO CHANGE]")
                     : "P2: PRESS [ENTER] TO LOCK")));
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
            sf::Text sub(*m_font, "Host is choosing stage. Waiting to start...", 13);
            sub.setStyle(sf::Text::Bold);
            sub.setFillColor(sf::Color(80, 220, 255));
            sub.setOrigin(sf::Vector2f(sub.getLocalBounds().size.x * 0.5f, 0.0f));
            sub.setPosition(sf::Vector2f(800.0f, 82.0f));
            window.draw(sub);
        } else {
            sf::Text sub(*m_font, "Select an arena with [A / D] or click. Press [ENTER] to start.", 12);
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
            { "TOPPAT AIRSHIP DECK", "Altitude: 14,000 ft", "High-altitude aircraft carrier deck flying through evening clouds.\nTailwinds and industrial steel railings.", sf::Color(245, 55, 65) },
            { "THE WALL - PRISON YARD", "Complex Sector 4", "High-security prison fortress during a snowstorm.\nConcrete courtyard, searchlight towers, and freezing winds.", sf::Color(80, 200, 255) },
            { "THE BANK VAULT", "Desert Canyon", "Desert bank vault breached with explosives.\nScattered gold bullion, canyon mesas, and dust storms.", sf::Color(255, 195, 45) }
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

        std::string p2TitleStr = (m_gameMode == GameMode::VsCpu)
                               ? ("CPU OPPONENT // " + std::string(getDifficultyName(m_cpuDifficulty)))
                               : p2Def.title;
        sf::Text p2Title(*m_font, p2TitleStr, 14);
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
    CpuDifficulty m_cpuDifficulty{ CpuDifficulty::Medium };

    size_t m_p1CharIdx{ 0 };
    size_t m_p2CharIdx{ 1 };
    bool m_p1Ready{ false };
    bool m_p2Ready{ false };

    int m_stageIdx{ 0 };
    int m_titleSelectedIdx{ 0 };
    int m_pauseSelectedIdx{ 0 };
    int m_cmdListTab{ 0 };

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
