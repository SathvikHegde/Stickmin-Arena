#include "RagdollEngine/Physics/RagdollSkeleton.hpp"
#include "RagdollEngine/Physics/PhysicsUnits.hpp"

namespace RagdollEngine {

RagdollSkeleton::RagdollSkeleton(b2WorldId worldId, int fighterId, const sf::Vector2f& spawnPosPixels)
    : m_worldId(worldId), m_fighterId(fighterId) {
    for (size_t i = 0; i < static_cast<size_t>(LimbType::Count); ++i) {
        m_bodies[i] = b2_nullBodyId;
    }
    for (size_t i = 0; i < static_cast<size_t>(JointType::Count); ++i) {
        m_joints[i] = b2_nullJointId;
    }
    assembleLimbs(spawnPosPixels);
}

RagdollSkeleton::~RagdollSkeleton() {
    if (!b2World_IsValid(m_worldId)) return;

    // Destroy joints first
    for (size_t i = 0; i < static_cast<size_t>(JointType::Count); ++i) {
        if (b2Joint_IsValid(m_joints[i])) {
            b2DestroyJoint(m_joints[i]);
            m_joints[i] = b2_nullJointId;
        }
    }

    // Destroy bodies
    for (size_t i = 0; i < static_cast<size_t>(LimbType::Count); ++i) {
        if (b2Body_IsValid(m_bodies[i])) {
            b2DestroyBody(m_bodies[i]);
            m_bodies[i] = b2_nullBodyId;
        }
    }
}

sf::Vector2f RagdollSkeleton::getPositionPixels() const {
    b2BodyId torso = getTorso();
    if (b2Body_IsValid(torso)) {
        b2Vec2 pos = b2Body_GetPosition(torso);
        return PhysicsUnits::toPixels(pos);
    }
    return sf::Vector2f(0.0f, 0.0f);
}

sf::Vector2f RagdollSkeleton::getLeftFistPixels() const {
    b2BodyId body = getBody(LimbType::LeftForearm);
    if (!b2Body_IsValid(body)) return getPositionPixels();
    b2Vec2 localTip = { 0.0f, PhysicsUnits::toMeters(FOREARM_LEN * 0.5f) };
    b2Vec2 world = b2Body_GetWorldPoint(body, localTip);
    return PhysicsUnits::toPixels(world);
}

sf::Vector2f RagdollSkeleton::getRightFistPixels() const {
    b2BodyId body = getBody(LimbType::RightForearm);
    if (!b2Body_IsValid(body)) return getPositionPixels();
    b2Vec2 localTip = { 0.0f, PhysicsUnits::toMeters(FOREARM_LEN * 0.5f) };
    b2Vec2 world = b2Body_GetWorldPoint(body, localTip);
    return PhysicsUnits::toPixels(world);
}

sf::Vector2f RagdollSkeleton::getLeftFootPixels() const {
    b2BodyId body = getBody(LimbType::LeftShin);
    if (!b2Body_IsValid(body)) return getPositionPixels();
    b2Vec2 localTip = { 0.0f, PhysicsUnits::toMeters(SHIN_LEN * 0.5f) };
    b2Vec2 world = b2Body_GetWorldPoint(body, localTip);
    return PhysicsUnits::toPixels(world);
}

sf::Vector2f RagdollSkeleton::getRightFootPixels() const {
    b2BodyId body = getBody(LimbType::RightShin);
    if (!b2Body_IsValid(body)) return getPositionPixels();
    b2Vec2 localTip = { 0.0f, PhysicsUnits::toMeters(SHIN_LEN * 0.5f) };
    b2Vec2 world = b2Body_GetWorldPoint(body, localTip);
    return PhysicsUnits::toPixels(world);
}

void RagdollSkeleton::assembleLimbs(const sf::Vector2f& spawnPosPixels) {
    b2Filter filter = b2DefaultFilter();
    // Negative group index ensures limbs belonging to this fighter never self-collide
    filter.groupIndex = -m_fighterId;

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter = filter;
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.8f;
    shapeDef.material.restitution = 0.05f;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.angularDamping = 1.5f; // Realistic rotational air resistance
    bodyDef.linearDamping = 0.3f;

    float cx = spawnPosPixels.x;
    float cy = spawnPosPixels.y;

    // 1. Torso (Center of skeleton)
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx, cy));
    b2BodyId torso = b2CreateBody(m_worldId, &bodyDef);
    b2Polygon torsoShape = b2MakeRoundedBox(PhysicsUnits::toMeters(TORSO_WIDTH * 0.5f), PhysicsUnits::toMeters(TORSO_HEIGHT * 0.5f), PhysicsUnits::toMeters(2.0f));
    shapeDef.density = 2.5f; // Torso has mass
    b2CreatePolygonShape(torso, &shapeDef, &torsoShape);
    m_bodies[static_cast<size_t>(LimbType::Torso)] = torso;

    // 2. Head
    float headY = cy - (TORSO_HEIGHT * 0.5f) - HEAD_RADIUS;
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx, headY));
    b2BodyId head = b2CreateBody(m_worldId, &bodyDef);
    b2Circle headShape;
    headShape.center = b2Vec2{ 0.0f, 0.0f };
    headShape.radius = PhysicsUnits::toMeters(HEAD_RADIUS);
    shapeDef.density = 1.2f;
    b2CreateCircleShape(head, &shapeDef, &headShape);
    m_bodies[static_cast<size_t>(LimbType::Head)] = head;

    // 3. Hips / Pelvis
    float hipsY = cy + (TORSO_HEIGHT * 0.5f) + (HIPS_HEIGHT * 0.5f);
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx, hipsY));
    b2BodyId hips = b2CreateBody(m_worldId, &bodyDef);
    b2Polygon hipsShape = b2MakeRoundedBox(PhysicsUnits::toMeters(HIPS_WIDTH * 0.5f), PhysicsUnits::toMeters(HIPS_HEIGHT * 0.5f), PhysicsUnits::toMeters(2.0f));
    shapeDef.density = 2.0f;
    b2CreatePolygonShape(hips, &shapeDef, &hipsShape);
    m_bodies[static_cast<size_t>(LimbType::Hips)] = hips;

    // 4. Arms
    float armY = cy - (TORSO_HEIGHT * 0.5f) + 4.0f;
    b2Polygon armShape = b2MakeRoundedBox(PhysicsUnits::toMeters(UPPER_ARM_WIDTH * 0.5f), PhysicsUnits::toMeters(UPPER_ARM_LEN * 0.5f), PhysicsUnits::toMeters(1.5f));
    b2Polygon forearmShape = b2MakeRoundedBox(PhysicsUnits::toMeters(FOREARM_WIDTH * 0.5f), PhysicsUnits::toMeters(FOREARM_LEN * 0.5f), PhysicsUnits::toMeters(1.5f));
    shapeDef.density = 0.8f;

    // Left Upper Arm
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx - 8.0f, armY + UPPER_ARM_LEN * 0.5f));
    b2BodyId leftUpperArm = b2CreateBody(m_worldId, &bodyDef);
    b2CreatePolygonShape(leftUpperArm, &shapeDef, &armShape);
    m_bodies[static_cast<size_t>(LimbType::LeftUpperArm)] = leftUpperArm;

    // Left Forearm
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx - 8.0f, armY + UPPER_ARM_LEN + FOREARM_LEN * 0.5f));
    b2BodyId leftForearm = b2CreateBody(m_worldId, &bodyDef);
    b2CreatePolygonShape(leftForearm, &shapeDef, &forearmShape);
    m_bodies[static_cast<size_t>(LimbType::LeftForearm)] = leftForearm;

    // Right Upper Arm
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx + 8.0f, armY + UPPER_ARM_LEN * 0.5f));
    b2BodyId rightUpperArm = b2CreateBody(m_worldId, &bodyDef);
    b2CreatePolygonShape(rightUpperArm, &shapeDef, &armShape);
    m_bodies[static_cast<size_t>(LimbType::RightUpperArm)] = rightUpperArm;

    // Right Forearm
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx + 8.0f, armY + UPPER_ARM_LEN + FOREARM_LEN * 0.5f));
    b2BodyId rightForearm = b2CreateBody(m_worldId, &bodyDef);
    b2CreatePolygonShape(rightForearm, &shapeDef, &forearmShape);
    m_bodies[static_cast<size_t>(LimbType::RightForearm)] = rightForearm;

    // 5. Legs
    float legY = hipsY + (HIPS_HEIGHT * 0.5f);
    b2Polygon thighShape = b2MakeRoundedBox(PhysicsUnits::toMeters(THIGH_WIDTH * 0.5f), PhysicsUnits::toMeters(THIGH_LEN * 0.5f), PhysicsUnits::toMeters(2.0f));
    b2Polygon shinShape = b2MakeRoundedBox(PhysicsUnits::toMeters(SHIN_WIDTH * 0.5f), PhysicsUnits::toMeters(SHIN_LEN * 0.5f), PhysicsUnits::toMeters(2.0f));
    shapeDef.density = 1.2f;
    shapeDef.material.friction = 0.9f; // High grip for feet

    // Left Thigh
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx - 5.0f, legY + THIGH_LEN * 0.5f));
    b2BodyId leftThigh = b2CreateBody(m_worldId, &bodyDef);
    b2CreatePolygonShape(leftThigh, &shapeDef, &thighShape);
    m_bodies[static_cast<size_t>(LimbType::LeftThigh)] = leftThigh;

    // Left Shin
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx - 5.0f, legY + THIGH_LEN + SHIN_LEN * 0.5f));
    b2BodyId leftShin = b2CreateBody(m_worldId, &bodyDef);
    b2CreatePolygonShape(leftShin, &shapeDef, &shinShape);
    m_bodies[static_cast<size_t>(LimbType::LeftShin)] = leftShin;

    // Right Thigh
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx + 5.0f, legY + THIGH_LEN * 0.5f));
    b2BodyId rightThigh = b2CreateBody(m_worldId, &bodyDef);
    b2CreatePolygonShape(rightThigh, &shapeDef, &thighShape);
    m_bodies[static_cast<size_t>(LimbType::RightThigh)] = rightThigh;

    // Right Shin
    bodyDef.position = PhysicsUnits::toMeters(sf::Vector2f(cx + 5.0f, legY + THIGH_LEN + SHIN_LEN * 0.5f));
    b2BodyId rightShin = b2CreateBody(m_worldId, &bodyDef);
    b2CreatePolygonShape(rightShin, &shapeDef, &shinShape);
    m_bodies[static_cast<size_t>(LimbType::RightShin)] = rightShin;

    // -----------------------------------------------------------------
    // JOINTS ASSEMBLY WITH BOX2D 3.1 NATIVE IMPLICIT SPRINGS
    // -----------------------------------------------------------------
    auto createJoint = [&](b2BodyId bodyA, b2BodyId bodyB, const sf::Vector2f& anchorPixels, float lowerAngleDeg, float upperAngleDeg, float springHertz = 14.0f) -> b2JointId {
        b2RevoluteJointDef jd = b2DefaultRevoluteJointDef();
        jd.bodyIdA = bodyA;
        jd.bodyIdB = bodyB;
        b2Vec2 anchorMeters = PhysicsUnits::toMeters(anchorPixels);
        jd.localAnchorA = b2Body_GetLocalPoint(bodyA, anchorMeters);
        jd.localAnchorB = b2Body_GetLocalPoint(bodyB, anchorMeters);
        jd.enableLimit = true;
        jd.lowerAngle = PhysicsUnits::degToRad(lowerAngleDeg);
        jd.upperAngle = PhysicsUnits::degToRad(upperAngleDeg);
        
        // Native Box2D 3 spring-damper
        jd.enableSpring = true;
        jd.hertz = springHertz;
        jd.dampingRatio = 0.8f;
        jd.targetAngle = 0.0f;
        jd.enableMotor = false;
        jd.maxMotorTorque = 150.0f;
        jd.motorSpeed = 0.0f;
        return b2CreateRevoluteJoint(m_worldId, &jd);
    };

    // Neck: Torso top to Head bottom
    m_joints[static_cast<size_t>(JointType::Neck)] = createJoint(torso, head, sf::Vector2f(cx, cy - (TORSO_HEIGHT * 0.5f)), -45.0f, 45.0f, 16.0f);

    // Spine: Torso bottom to Hips top (Strong core spring)
    m_joints[static_cast<size_t>(JointType::Spine)] = createJoint(torso, hips, sf::Vector2f(cx, cy + (TORSO_HEIGHT * 0.5f)), -35.0f, 35.0f, 18.0f);

    // Shoulders (Natural 360-degree martial arts flexibility)
    m_joints[static_cast<size_t>(JointType::LeftShoulder)] = createJoint(torso, leftUpperArm, sf::Vector2f(cx - 8.0f, armY), -175.0f, 175.0f, 14.0f);
    m_joints[static_cast<size_t>(JointType::RightShoulder)] = createJoint(torso, rightUpperArm, sf::Vector2f(cx + 8.0f, armY), -175.0f, 175.0f, 14.0f);

    // Elbows (Full natural range for both arms in either facing direction)
    m_joints[static_cast<size_t>(JointType::LeftElbow)] = createJoint(leftUpperArm, leftForearm, sf::Vector2f(cx - 8.0f, armY + UPPER_ARM_LEN), -160.0f, 160.0f, 16.0f);
    m_joints[static_cast<size_t>(JointType::RightElbow)] = createJoint(rightUpperArm, rightForearm, sf::Vector2f(cx + 8.0f, armY + UPPER_ARM_LEN), -160.0f, 160.0f, 16.0f);

    // Hips to Thighs (Wide athletic base without crossing)
    m_joints[static_cast<size_t>(JointType::LeftHip)] = createJoint(hips, leftThigh, sf::Vector2f(cx - 5.0f, legY), -120.0f, 120.0f, 16.0f);
    m_joints[static_cast<size_t>(JointType::RightHip)] = createJoint(hips, rightThigh, sf::Vector2f(cx + 5.0f, legY), -120.0f, 120.0f, 16.0f);

    // Knees (Natural bending range for both legs in either facing direction)
    m_joints[static_cast<size_t>(JointType::LeftKnee)] = createJoint(leftThigh, leftShin, sf::Vector2f(cx - 5.0f, legY + THIGH_LEN), -150.0f, 150.0f, 16.0f);
    m_joints[static_cast<size_t>(JointType::RightKnee)] = createJoint(rightThigh, rightShin, sf::Vector2f(cx + 5.0f, legY + THIGH_LEN), -150.0f, 150.0f, 16.0f);
}

} // namespace RagdollEngine
