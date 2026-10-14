#include "MenuScene.hpp"
#include "GameContext.hpp"
#include "SceneManager.hpp"
#include "MenuManager.hpp"
#include "NetworkManager.hpp"

#include <RagdollEngine/Render/StageRenderer.hpp>
#include <RagdollEngine/Core/TimeManager.hpp>

namespace StickminGame {

MenuScene::MenuScene(GameContext& ctx)
    : m_ctx(ctx) {
}

void MenuScene::onEnter() {
    // If coming from Battle, ensure menu state is clean
    if (m_ctx.menuManager.getState() == MenuState::Battle ||
        m_ctx.menuManager.getState() == MenuState::PauseMenu) {
        m_ctx.menuManager.setState(MenuState::CharacterSelect);
    }
}

void MenuScene::onExit() {
}

void MenuScene::handleEvent(const sf::Event& event) {
    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2i pixel(mouseMoved->position.x, mouseMoved->position.y);
        sf::Vector2f uiMousePos = m_ctx.window.mapPixelToCoords(pixel, m_ctx.uiView);
        m_ctx.menuManager.handleMouseMove(uiMousePos);
    }

    if (const auto* textEntered = event.getIf<sf::Event::TextEntered>()) {
        m_ctx.menuManager.handleTextEntered(textEntered->unicode);
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        m_ctx.menuManager.handleKeyPressed(keyPressed->code);
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        sf::Vector2i pixel(mousePressed->position.x, mousePressed->position.y);
        sf::Vector2f uiMousePos = m_ctx.window.mapPixelToCoords(pixel, m_ctx.uiView);
        m_ctx.menuManager.handleMouseClick(uiMousePos, mousePressed->button);
    }
}

void MenuScene::update(float dt) {
    m_ctx.menuManager.update(dt);
    m_ctx.netManager.update(dt);

    // Online Lobby & Lifecycle Synchronization
    if (m_ctx.menuManager.getGameMode() == GameMode::Online && m_ctx.netManager.isConnected()) {
        LobbySyncData peerLobby;
        if (m_ctx.netManager.consumeLobbySync(peerLobby)) {
            if (m_ctx.netManager.isClient()) {
                m_ctx.menuManager.setP1CharIndex(peerLobby.p1CharIdx);
                m_ctx.menuManager.setP1Ready(peerLobby.p1Ready);
                m_ctx.menuManager.setSelectedStageIndex(peerLobby.stageIdx);
                m_ctx.stageRenderer.setStage(static_cast<RagdollEngine::StageType>(peerLobby.stageIdx));
                if (m_ctx.menuManager.getState() == MenuState::OnlineLobbyModal) {
                    m_ctx.menuManager.setState(MenuState::CharacterSelect);
                }
                if (m_ctx.menuManager.getState() == MenuState::StageSelect && (!peerLobby.p1Ready || !peerLobby.p2Ready)) {
                    m_ctx.menuManager.setP2Ready(false);
                    m_ctx.menuManager.setState(MenuState::CharacterSelect);
                }
            } else if (m_ctx.netManager.isHost()) {
                m_ctx.menuManager.setP2CharIndex(peerLobby.p2CharIdx);
                m_ctx.menuManager.setP2Ready(peerLobby.p2Ready);
                if (m_ctx.menuManager.getState() == MenuState::OnlineLobbyModal) {
                    m_ctx.menuManager.setState(MenuState::CharacterSelect);
                }
                if (m_ctx.menuManager.getState() == MenuState::StageSelect && (!peerLobby.p1Ready || !peerLobby.p2Ready)) {
                    m_ctx.menuManager.setP1Ready(false);
                    m_ctx.menuManager.setState(MenuState::CharacterSelect);
                }
            }
        }

        if (m_ctx.menuManager.consumeLobbyDirty()) {
            LobbySyncData myLobby;
            myLobby.p1CharIdx = static_cast<uint8_t>(m_ctx.menuManager.getP1CharIndex());
            myLobby.p2CharIdx = static_cast<uint8_t>(m_ctx.menuManager.getP2CharIndex());
            myLobby.stageIdx = static_cast<uint8_t>(m_ctx.menuManager.getSelectedStageIndex());
            myLobby.p1Ready = m_ctx.menuManager.isP1Ready();
            myLobby.p2Ready = m_ctx.menuManager.isP2Ready();
            m_ctx.netManager.sendLobbySync(myLobby);
        }

        MatchStartData startData;
        if (m_ctx.netManager.isClient() && m_ctx.netManager.consumeMatchStart(startData)) {
            m_ctx.menuManager.setSelectedStageIndex(startData.stageIdx);
            m_ctx.menuManager.setP1CharIndex(startData.p1CharIdx);
            m_ctx.menuManager.setP2CharIndex(startData.p2CharIdx);
            m_ctx.stageRenderer.setStage(static_cast<RagdollEngine::StageType>(startData.stageIdx));
            m_ctx.menuManager.setState(MenuState::VersusIntro);
        }
    }

    // Handle match transition when Versus Intro ends or start requested
    if (m_ctx.menuManager.consumeStartMatchRequested()) {
        m_ctx.sceneManager.setScene(SceneType::Battle);
        return;
    }

    if (m_ctx.menuManager.getState() == MenuState::StageSelect) {
        m_ctx.stageRenderer.update(dt);
    }

    if (m_ctx.menuManager.consumeQuitRequested()) {
        m_ctx.window.close();
    }
}

void MenuScene::render(sf::RenderWindow& window) {
    window.setView(m_ctx.uiView);

    if (m_ctx.menuManager.getState() == MenuState::StageSelect) {
        m_ctx.stageRenderer.drawBackground(window, m_ctx.uiView);
        m_ctx.stageRenderer.drawAtmosphereAndLighting(window, m_ctx.uiView);
        m_ctx.menuManager.draw(window);
    } else {
        m_ctx.menuManager.draw(window);
    }
}

} // namespace StickminGame
