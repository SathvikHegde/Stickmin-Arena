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
    void setState(MenuState s);

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


    // Lifecycle & Input
    void update(float dt);
    void handleKeyPressed(sf::Keyboard::Key key);
    void handleMouseMove(const sf::Vector2f& mousePos);
    void handleTextEntered(char32_t unicode);
    bool handleMouseClick(const sf::Vector2f& mousePos, sf::Mouse::Button button);
    void draw(sf::RenderWindow& window);

private:
    void executeTitleAction(int idx);
    void executePauseAction(int idx);
    void drawTitleScreen(sf::RenderWindow& window);
    void drawOnlineLobbyModal(sf::RenderWindow& window);
    void drawCommandListModal(sf::RenderWindow& window);
    void drawCharacterSelect(sf::RenderWindow& window);
    void drawPlayerProfileCard(sf::RenderWindow& window, int playerNum, size_t charIdx, bool isReady, const sf::Vector2f& pos, const sf::Vector2f& size);
    void drawStageSelect(sf::RenderWindow& window);
    void drawVersusIntro(sf::RenderWindow& window);
    void drawPauseMenu(sf::RenderWindow& window);

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
