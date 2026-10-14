#pragma once
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

namespace StickminGame {

enum class SceneType {
    Menu,
    Battle
};

class IScene {
public:
    virtual ~IScene() = default;

    virtual void onEnter() {}
    virtual void onExit() {}

    virtual void handleEvent(const sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderWindow& window) = 0;
};

} // namespace StickminGame
