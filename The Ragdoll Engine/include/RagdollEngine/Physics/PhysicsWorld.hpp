#pragma once
#include <box2d/box2d.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <vector>

namespace RagdollEngine {

struct StaticPlatform {
    b2BodyId bodyId;
    float widthPixels;
    float heightPixels;
    sf::Vector2f positionPixels;
};

class PhysicsWorld {
public:
    PhysicsWorld(float gravityY = 24.0f);
    ~PhysicsWorld();

    void step(float dt);

    // Helpers to create arena geometry
    b2BodyId createStaticBox(float xPixels, float yPixels, float widthPixels, float heightPixels, float friction = 0.8f);
    b2BodyId createDynamicBox(float xPixels, float yPixels, float widthPixels, float heightPixels, float density = 1.0f, float friction = 0.5f);

    b2WorldId getB2WorldId() const { return m_worldId; }
    const std::vector<StaticPlatform>& getPlatforms() const { return m_platforms; }

private:
    b2WorldId m_worldId;
    std::vector<StaticPlatform> m_platforms;
    int m_subStepCount{ 4 };
};

} // namespace RagdollEngine
