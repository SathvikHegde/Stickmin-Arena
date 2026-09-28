#pragma once
#include <SFML/System/Vector2.hpp>
#include <box2d/box2d.h>

namespace RagdollEngine {

class PhysicsUnits {
public:
    static constexpr float PPM = 35.0f; // Pixels per meter ratio

    static inline float toPixels(float meters) {
        return meters * PPM;
    }

    static inline float toMeters(float pixels) {
        return pixels / PPM;
    }

    static inline sf::Vector2f toPixels(const b2Vec2& metersVec) {
        return sf::Vector2f(metersVec.x * PPM, metersVec.y * PPM);
    }

    static inline b2Vec2 toMeters(const sf::Vector2f& pixelVec) {
        return b2Vec2{ pixelVec.x / PPM, pixelVec.y / PPM };
    }

    static inline float radToDeg(float radians) {
        return radians * (180.0f / 3.14159265358979323846f);
    }

    static inline float degToRad(float degrees) {
        return degrees * (3.14159265358979323846f / 180.0f);
    }
};

} // namespace RagdollEngine
