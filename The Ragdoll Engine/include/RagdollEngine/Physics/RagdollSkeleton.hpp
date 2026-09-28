#pragma once
#include <box2d/box2d.h>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <string>

namespace RagdollEngine {

enum class LimbType {
    Head,
    Torso,
    Hips,
    LeftUpperArm,
    LeftForearm,
    RightUpperArm,
    RightForearm,
    LeftThigh,
    LeftShin,
    RightThigh,
    RightShin,
    Count
};

enum class JointType {
    Neck,
    Spine,
    LeftShoulder,
    LeftElbow,
    RightShoulder,
    RightElbow,
    LeftHip,
    LeftKnee,
    RightHip,
    RightKnee,
    Count
};

class RagdollSkeleton {
public:
    RagdollSkeleton(b2WorldId worldId, int fighterId, const sf::Vector2f& spawnPosPixels);
    ~RagdollSkeleton();

    // Disable copy, enable move if needed
    RagdollSkeleton(const RagdollSkeleton&) = delete;
    RagdollSkeleton& operator=(const RagdollSkeleton&) = delete;

    // Body accessors
    b2BodyId getBody(LimbType limb) const { return m_bodies[static_cast<size_t>(limb)]; }
    b2JointId getJoint(JointType joint) const { return m_joints[static_cast<size_t>(joint)]; }

    // Primary representative body for position / tracking (Torso or Hips)
    b2BodyId getTorso() const { return m_bodies[static_cast<size_t>(LimbType::Torso)]; }
    b2BodyId getHips() const { return m_bodies[static_cast<size_t>(LimbType::Hips)]; }
    b2BodyId getHead() const { return m_bodies[static_cast<size_t>(LimbType::Head)]; }

    sf::Vector2f getPositionPixels() const;
    int getFighterId() const { return m_fighterId; }

    // Distal limb contact points (in pixel coordinates)
    sf::Vector2f getLeftFistPixels() const;
    sf::Vector2f getRightFistPixels() const;
    sf::Vector2f getLeftFootPixels() const;
    sf::Vector2f getRightFootPixels() const;

    // Physical dimensions (in pixels)
    static constexpr float HEAD_RADIUS = 13.0f;
    static constexpr float TORSO_WIDTH = 8.0f;
    static constexpr float TORSO_HEIGHT = 28.0f;
    static constexpr float HIPS_WIDTH = 10.0f;
    static constexpr float HIPS_HEIGHT = 10.0f;
    static constexpr float UPPER_ARM_LEN = 20.0f;
    static constexpr float UPPER_ARM_WIDTH = 5.0f;
    static constexpr float FOREARM_LEN = 18.0f;
    static constexpr float FOREARM_WIDTH = 4.5f;
    static constexpr float THIGH_LEN = 24.0f;
    static constexpr float THIGH_WIDTH = 6.0f;
    static constexpr float SHIN_LEN = 22.0f;
    static constexpr float SHIN_WIDTH = 5.5f;

private:
    b2WorldId m_worldId;
    int m_fighterId{ 1 };

    b2BodyId m_bodies[static_cast<size_t>(LimbType::Count)];
    b2JointId m_joints[static_cast<size_t>(JointType::Count)];

    void assembleLimbs(const sf::Vector2f& spawnPosPixels);
};

} // namespace RagdollEngine
