#pragma once
#include "RagdollEngine/Physics/RagdollSkeleton.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Color.hpp>

namespace RagdollEngine {

struct FighterVisualTheme {
    sf::Color bodyColor{ sf::Color(25, 25, 30) };        // Sharp dark stick body
    sf::Color headFillColor{ sf::Color(245, 245, 250) };   // White head
    sf::Color headOutlineColor{ sf::Color(20, 20, 25) };  // Crisp black outline
    sf::Color accentColor{ sf::Color(230, 45, 65) };      // Headband / belt accent (P1 = Red)
    float lineThickness{ 5.5f };
    bool glowingEyes{ false };
    sf::Color eyeGlowColor{ sf::Color(255, 220, 60) };
};

class RagdollRenderer {
public:
    RagdollRenderer();

    void draw(sf::RenderWindow& window, const RagdollSkeleton& skeleton, int facingDir, const FighterVisualTheme& theme);
    void drawDropShadow(sf::RenderWindow& window, const RagdollSkeleton& skeleton, float groundY = 800.0f);

private:
    void drawLimbSegment(sf::RenderWindow& window, b2BodyId body, float halfWidthPixels, float halfHeightPixels, const sf::Color& color);
    void drawHead(sf::RenderWindow& window, b2BodyId headBody, int facingDir, const FighterVisualTheme& theme);
};

} // namespace RagdollEngine
