#include "SceneManager.hpp"
#include "GameContext.hpp"
#include "MenuScene.hpp"
#include "BattleScene.hpp"

namespace StickminGame {

SceneManager::SceneManager(GameContext* ctx)
    : m_ctx(ctx) {
}

SceneManager::~SceneManager() = default;

void SceneManager::setScene(SceneType type) {
    m_pendingSceneType = type;
}

void SceneManager::applyPendingScene() {
    if (!m_pendingSceneType.has_value()) return;

    SceneType nextType = *m_pendingSceneType;
    m_pendingSceneType.reset();

    if (m_currentScene) {
        m_currentScene->onExit();
    }

    m_currentScene = createScene(nextType);
    m_currentType = nextType;

    if (m_currentScene) {
        m_currentScene->onEnter();
    }
}

std::unique_ptr<IScene> SceneManager::createScene(SceneType type) {
    if (!m_ctx) return nullptr;

    switch (type) {
        case SceneType::Menu:
            return std::make_unique<MenuScene>(*m_ctx);
        case SceneType::Battle:
            return std::make_unique<BattleScene>(*m_ctx);
        default:
            return nullptr;
    }
}

void SceneManager::handleEvent(const sf::Event& event) {
    if (m_currentScene) {
        m_currentScene->handleEvent(event);
    }
}

void SceneManager::update(float dt) {
    applyPendingScene();
    if (m_currentScene) {
        m_currentScene->update(dt);
    }
    applyPendingScene();
}

void SceneManager::render(sf::RenderWindow& window) {
    if (m_currentScene) {
        m_currentScene->render(window);
    }
}

} // namespace StickminGame
