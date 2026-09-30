#pragma once
#include "RagdollEngine/Physics/RagdollSkeleton.hpp"
#include "RagdollEngine/Render/CharacterDef.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

namespace RagdollEngine {

using FighterVisualTheme = CharacterDefinition;

class RagdollRenderer {
public:
    RagdollRenderer();

    void draw(sf::RenderWindow& window, const RagdollSkeleton& skeleton, int facingDir, const CharacterDefinition& character);
    void drawDropShadow(sf::RenderWindow& window, const RagdollSkeleton& skeleton, float groundY = 800.0f);

private:
    void drawLimbSegment(sf::RenderWindow& window, b2BodyId body, float halfWidthPixels, float halfHeightPixels, const sf::Color& color);
    void drawShoe(sf::RenderWindow& window, b2BodyId shinBody, int facingDir, const CharacterDefinition& character);
    void drawHand(sf::RenderWindow& window, b2BodyId forearmBody, int facingDir, const CharacterDefinition& character);
    
    // Head & Accessories
    void drawHead(sf::RenderWindow& window, b2BodyId headBody, int facingDir, const CharacterDefinition& character);
    void drawBackCosmetics(sf::RenderWindow& window, const sf::Vector2f& headPos, float headRad, int facingDir, const CharacterDefinition& character);
    void drawFrontCosmetics(sf::RenderWindow& window, const sf::Vector2f& headPos, float headRad, int facingDir, const CharacterDefinition& character);
    void drawFace(sf::RenderWindow& window, const sf::Vector2f& headPos, float headRad, float headAngleRad, int facingDir, const CharacterDefinition& character);
};

} // namespace RagdollEngine
