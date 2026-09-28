#include "RagdollEngine/Physics/PhysicsWorld.hpp"
#include "RagdollEngine/Physics/PhysicsUnits.hpp"

namespace RagdollEngine {

PhysicsWorld::PhysicsWorld(float gravityY) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2{ 0.0f, gravityY };
    m_worldId = b2CreateWorld(&worldDef);
}

PhysicsWorld::~PhysicsWorld() {
    if (b2World_IsValid(m_worldId)) {
        b2DestroyWorld(m_worldId);
    }
}

void PhysicsWorld::step(float dt) {
    if (b2World_IsValid(m_worldId)) {
        b2World_Step(m_worldId, dt, m_subStepCount);
    }
}

b2BodyId PhysicsWorld::createStaticBox(float xPixels, float yPixels, float widthPixels, float heightPixels, float friction) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(xPixels, yPixels));

    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 0.0f;
    shapeDef.material.friction = friction;
    shapeDef.material.restitution = 0.1f;

    b2Polygon box = b2MakeBox(PhysicsUnits::toMeters(widthPixels * 0.5f), PhysicsUnits::toMeters(heightPixels * 0.5f));
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    m_platforms.push_back({ bodyId, widthPixels, heightPixels, sf::Vector2f(xPixels, yPixels) });
    return bodyId;
}

b2BodyId PhysicsWorld::createDynamicBox(float xPixels, float yPixels, float widthPixels, float heightPixels, float density, float friction) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(xPixels, yPixels));

    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = density;
    shapeDef.material.friction = friction;
    shapeDef.material.restitution = 0.3f;

    b2Polygon box = b2MakeBox(PhysicsUnits::toMeters(widthPixels * 0.5f), PhysicsUnits::toMeters(heightPixels * 0.5f));
    b2CreatePolygonShape(bodyId, &shapeDef, &box);
    return bodyId;
}

} // namespace RagdollEngine
