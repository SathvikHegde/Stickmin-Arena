#pragma once
#include "IScene.hpp"
#include <memory>
#include <optional>

namespace StickminGame {

struct GameContext;

class SceneManager {
public:
    explicit SceneManager(GameContext* ctx = nullptr);
    ~SceneManager();

    void setContext(GameContext* ctx) { m_ctx = ctx; }
    void setScene(SceneType type);

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderWindow& window);

    SceneType getCurrentSceneType() const { return m_currentType; }
    IScene* getCurrentScene() const { return m_currentScene.get(); }

private:
    GameContext* m_ctx{ nullptr };
    std::unique_ptr<IScene> m_currentScene;
    SceneType m_currentType{ SceneType::Menu };
    std::optional<SceneType> m_pendingSceneType;

    void applyPendingScene();
    std::unique_ptr<IScene> createScene(SceneType type);
};

} // namespace StickminGame
