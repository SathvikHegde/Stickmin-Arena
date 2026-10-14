#pragma once
#include "IScene.hpp"

namespace StickminGame {

struct GameContext;

class MenuScene : public IScene {
public:
    explicit MenuScene(GameContext& ctx);
    ~MenuScene() override = default;

    void onEnter() override;
    void onExit() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    GameContext& m_ctx;
};

} // namespace StickminGame
