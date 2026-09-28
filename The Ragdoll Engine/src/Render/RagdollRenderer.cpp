#include "RagdollEngine/Render/RagdollRenderer.hpp"
#include "RagdollEngine/Physics/PhysicsUnits.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>
#include <algorithm>

namespace RagdollEngine {

RagdollRenderer::RagdollRenderer() = default;

void RagdollRenderer::drawLimbSegment(sf::RenderWindow& window, b2BodyId body, float halfWidthPixels, float halfHeightPixels, const sf::Color& color) {
    if (!b2Body_IsValid(body)) return;

    sf::Vector2f pos = PhysicsUnits::toPixels(b2Body_GetPosition(body));
    b2Rot rot = b2Body_GetRotation(body);
    float rad = b2Rot_GetAngle(rot);

    // Main capsule rectangle
    sf::RectangleShape rect(sf::Vector2f(halfWidthPixels * 2.0f, halfHeightPixels * 2.0f));
    rect.setOrigin(sf::Vector2f(halfWidthPixels, halfHeightPixels));
    rect.setPosition(pos);
    rect.setRotation(sf::radians(rad));
    rect.setFillColor(color);
    window.draw(rect);

    // Rounded end caps - properly oriented in screen space (SFML: +Y down, rotation clockwise)
    float sinA = std::sin(rad) * halfHeightPixels;
    float cosA = std::cos(rad) * halfHeightPixels;

    sf::CircleShape capTop(halfWidthPixels);
    capTop.setOrigin(sf::Vector2f(halfWidthPixels, halfWidthPixels));
    capTop.setFillColor(color);
    capTop.setPosition(sf::Vector2f(pos.x + sinA, pos.y - cosA));
    window.draw(capTop);

    sf::CircleShape capBottom(halfWidthPixels);
    capBottom.setOrigin(sf::Vector2f(halfWidthPixels, halfWidthPixels));
    capBottom.setFillColor(color);
    capBottom.setPosition(sf::Vector2f(pos.x - sinA, pos.y + cosA));
    window.draw(capBottom);
}

void RagdollRenderer::drawHead(sf::RenderWindow& window, b2BodyId headBody, int facingDir, const FighterVisualTheme& theme) {
    if (!b2Body_IsValid(headBody)) return;

    sf::Vector2f pos = PhysicsUnits::toPixels(b2Body_GetPosition(headBody));
    b2Rot rot = b2Body_GetRotation(headBody);
    float rad = b2Rot_GetAngle(rot);
    float radius = RagdollSkeleton::HEAD_RADIUS;

    // 1. Headband fluttering ribbon tails (flowing backwards away from facing direction)
    float tailOriginX = pos.x - (facingDir * radius * 0.9f);
    float tailOriginY = pos.y - 3.0f;
    
    sf::RectangleShape ribbon1(sf::Vector2f(14.0f, 3.5f));
    ribbon1.setOrigin(sf::Vector2f(0.0f, 1.75f));
    ribbon1.setPosition(sf::Vector2f(tailOriginX, tailOriginY));
    ribbon1.setRotation(sf::degrees(facingDir > 0 ? 195.0f : -15.0f));
    ribbon1.setFillColor(theme.accentColor);
    window.draw(ribbon1);

    sf::RectangleShape ribbon2(sf::Vector2f(10.0f, 2.5f));
    ribbon2.setOrigin(sf::Vector2f(0.0f, 1.25f));
    ribbon2.setPosition(sf::Vector2f(tailOriginX, tailOriginY + 2.0f));
    ribbon2.setRotation(sf::degrees(facingDir > 0 ? 215.0f : -35.0f));
    ribbon2.setFillColor(sf::Color(theme.accentColor.r, theme.accentColor.g, theme.accentColor.b, 200));
    window.draw(ribbon2);

    // 2. Head circle (Crisp border and fill)
    sf::CircleShape headCircle(radius);
    headCircle.setOrigin(sf::Vector2f(radius, radius));
    headCircle.setPosition(pos);
    headCircle.setRotation(sf::radians(rad));
    headCircle.setFillColor(theme.headFillColor);
    headCircle.setOutlineColor(theme.headOutlineColor);
    headCircle.setOutlineThickness(3.0f);
    window.draw(headCircle);

    // 3. Headband ribbon accent across forehead
    sf::RectangleShape headband(sf::Vector2f(radius * 2.0f + 2.0f, 6.0f));
    headband.setOrigin(sf::Vector2f(radius + 1.0f, 3.0f));
    float bandOffsetY = 4.0f;
    float bandWorldX = pos.x + std::sin(rad) * bandOffsetY;
    float bandWorldY = pos.y - std::cos(rad) * bandOffsetY;
    headband.setPosition(sf::Vector2f(bandWorldX, bandWorldY));
    headband.setRotation(sf::radians(rad));
    headband.setFillColor(theme.accentColor);
    window.draw(headband);

    // 4. Expressive Eye (or Glowing Rage Eye)
    float eyeDist = 4.5f * facingDir;
    float eyeOffsetY = 1.0f;
    float eyeWorldX = pos.x + (eyeDist * std::cos(rad) + eyeOffsetY * std::sin(rad));
    float eyeWorldY = pos.y + (eyeDist * std::sin(rad) - eyeOffsetY * std::cos(rad));

    if (theme.glowingEyes) {
        sf::CircleShape glow(4.5f);
        glow.setOrigin(sf::Vector2f(4.5f, 4.5f));
        glow.setPosition(sf::Vector2f(eyeWorldX, eyeWorldY));
        glow.setFillColor(sf::Color(theme.eyeGlowColor.r, theme.eyeGlowColor.g, theme.eyeGlowColor.b, 120));
        window.draw(glow);

        sf::CircleShape eye(2.4f);
        eye.setOrigin(sf::Vector2f(2.4f, 2.4f));
        eye.setPosition(sf::Vector2f(eyeWorldX, eyeWorldY));
        eye.setFillColor(theme.eyeGlowColor);
        window.draw(eye);
    } else {
        sf::CircleShape eye(2.2f);
        eye.setOrigin(sf::Vector2f(2.2f, 2.2f));
        eye.setPosition(sf::Vector2f(eyeWorldX, eyeWorldY));
        eye.setFillColor(theme.headOutlineColor);
        window.draw(eye);
    }
}

void RagdollRenderer::drawDropShadow(sf::RenderWindow& window, const RagdollSkeleton& skeleton, float groundY) {
    sf::Vector2f pos = skeleton.getPositionPixels();
    float heightAboveGround = std::max(0.0f, groundY - pos.y);
    float t = std::clamp(heightAboveGround / 250.0f, 0.0f, 1.0f);

    float radiusX = 26.0f * (1.0f - t * 0.45f);
    float radiusY = 7.0f * (1.0f - t * 0.45f);
    std::uint8_t alpha = static_cast<std::uint8_t>((1.0f - t) * 110.0f);

    if (alpha > 5) {
        sf::CircleShape shadow(radiusX);
        shadow.setScale(sf::Vector2f(1.0f, radiusY / radiusX));
        shadow.setOrigin(sf::Vector2f(radiusX, radiusX));
        shadow.setPosition(sf::Vector2f(pos.x, groundY));
        shadow.setFillColor(sf::Color(0, 0, 0, alpha));
        window.draw(shadow);
    }
}

void RagdollRenderer::draw(sf::RenderWindow& window, const RagdollSkeleton& skeleton, int facingDir, const FighterVisualTheme& theme) {
    // 1. Draw back limbs (left arm & left leg)
    drawLimbSegment(window, skeleton.getBody(LimbType::LeftThigh), RagdollSkeleton::THIGH_WIDTH * 0.5f, RagdollSkeleton::THIGH_LEN * 0.5f, theme.bodyColor);
    drawLimbSegment(window, skeleton.getBody(LimbType::LeftShin), RagdollSkeleton::SHIN_WIDTH * 0.5f, RagdollSkeleton::SHIN_LEN * 0.5f, theme.bodyColor);

    drawLimbSegment(window, skeleton.getBody(LimbType::LeftUpperArm), RagdollSkeleton::UPPER_ARM_WIDTH * 0.5f, RagdollSkeleton::UPPER_ARM_LEN * 0.5f, theme.bodyColor);
    drawLimbSegment(window, skeleton.getBody(LimbType::LeftForearm), RagdollSkeleton::FOREARM_WIDTH * 0.5f, RagdollSkeleton::FOREARM_LEN * 0.5f, theme.bodyColor);

    // 2. Draw Torso & Hips
    drawLimbSegment(window, skeleton.getBody(LimbType::Hips), RagdollSkeleton::HIPS_WIDTH * 0.5f, RagdollSkeleton::HIPS_HEIGHT * 0.5f, theme.bodyColor);
    drawLimbSegment(window, skeleton.getBody(LimbType::Torso), RagdollSkeleton::TORSO_WIDTH * 0.5f, RagdollSkeleton::TORSO_HEIGHT * 0.5f, theme.bodyColor);

    // 3. Draw front limbs (right leg & right arm)
    drawLimbSegment(window, skeleton.getBody(LimbType::RightThigh), RagdollSkeleton::THIGH_WIDTH * 0.5f, RagdollSkeleton::THIGH_LEN * 0.5f, theme.bodyColor);
    drawLimbSegment(window, skeleton.getBody(LimbType::RightShin), RagdollSkeleton::SHIN_WIDTH * 0.5f, RagdollSkeleton::SHIN_LEN * 0.5f, theme.bodyColor);

    drawLimbSegment(window, skeleton.getBody(LimbType::RightUpperArm), RagdollSkeleton::UPPER_ARM_WIDTH * 0.5f, RagdollSkeleton::UPPER_ARM_LEN * 0.5f, theme.bodyColor);
    drawLimbSegment(window, skeleton.getBody(LimbType::RightForearm), RagdollSkeleton::FOREARM_WIDTH * 0.5f, RagdollSkeleton::FOREARM_LEN * 0.5f, theme.bodyColor);

    // 4. Draw Head
    drawHead(window, skeleton.getBody(LimbType::Head), facingDir, theme);
}

} // namespace RagdollEngine
