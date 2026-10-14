#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/View.hpp>

namespace RagdollEngine {
    class TimeManager;
    class RagdollRenderer;
    class StageRenderer;
}

namespace StickminGame {

class NetworkManager;
class MenuManager;
class SceneManager;

struct GameContext {
    sf::RenderWindow& window;
    RagdollEngine::TimeManager& timeManager;
    RagdollEngine::RagdollRenderer& ragdollRenderer;
    RagdollEngine::StageRenderer& stageRenderer;
    NetworkManager& netManager;
    MenuManager& menuManager;
    const sf::Font* font{ nullptr };
    sf::View& uiView;
    SceneManager& sceneManager;
};

} // namespace StickminGame
