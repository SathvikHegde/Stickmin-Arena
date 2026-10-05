#pragma once
#include "RagdollEngine/Physics/RagdollSkeleton.hpp"
#include <box2d/box2d.h>
#include <string>
#include <algorithm>

namespace RagdollEngine {

enum class FighterActionState {
    Neutral,
    MovingForward,
    MovingBackward,
    Dashing,
    Crouching,
    HighGuarding,
    LowGuarding,
    Attacking,
    HitStun,
    LaunchedJuggle,
    KnockedDown,
    GettingUp,
    Limp
};

enum class MoveId {
    None,
    FlashJab,           // 1 (LP) - 10-frame high jab (plus on block)
    StraightCross,      // 2 (RP) - 12-frame mid straight punch
    OneTwoString,       // 1, 2 - Classic Tekken jab-cross string
    MidKick,            // 3 (LK) - Mid snap kick
    AxeRoundhouse,      // 4 (RK) - High roundhouse (Counter-hit launcher)
    ElectricWindGodFist,// f+2 - Mishima EWGF electric launcher!
    HellSweep,          // d+4 - Low sweep leg trip
    Hopkick,            // u/f+4 - Universal mid hopkick launcher!
    FlyingDropkick,     // f,f+3 / Running 3 - Flying dropkick
    PowerCrush,         // 1+2 - Armored shoulder rush (absorbs high/mids)
    Throw,              // 1+3 - Unblockable command grab and slam!
    RageArt             // 1+2 in Rage - Cinematic super attack!
};

enum class AttackHeight {
    High,
    Mid,
    Low
};

enum class HitResult {
    Miss,
    Blocked,
    CleanHit,
    CounterHit,
    PowerCrushAbsorb,   // Absorbed hit through armor!
    ThrowGrabbed,       // Caught in unblockable throw!
    ThrowBroken         // Successfully broke the throw!
};

struct MoveDefinition {
    MoveId id{ MoveId::None };
    std::string name{ "None" };
    AttackHeight height{ AttackHeight::Mid };
    float startup{ 0.08f };
    float active{ 0.06f };
    float recovery{ 0.12f };
    float damage{ 12.0f };
    bool isLauncher{ false };
    bool isTrip{ false };
    bool isElectric{ false };
    bool isPowerCrush{ false };
    bool isThrow{ false };
    bool isRageArt{ false };
    b2Vec2 launchImpulse{ 3.5f, -14.0f };
    LimbType strikingLimb{ LimbType::RightForearm };

    float totalDuration() const { return startup + active + recovery; }
};

struct RagdollPose {
    // Joint target angles in degrees
    float neck{ 0.0f };
    float spine{ 0.0f };
    float leftShoulder{ 20.0f };
    float leftElbow{ -70.0f };
    float rightShoulder{ -20.0f };
    float rightElbow{ 70.0f };
    float leftHip{ -10.0f };
    float leftKnee{ -15.0f };
    float rightHip{ 10.0f };
    float rightKnee{ 15.0f };

    void setFrontArm(int facingDir, float shoulder, float elbow) {
        if (facingDir > 0) {
            rightShoulder = shoulder;
            rightElbow = elbow;
        } else {
            leftShoulder = shoulder;
            leftElbow = elbow;
        }
    }

    void setRearArm(int facingDir, float shoulder, float elbow) {
        if (facingDir > 0) {
            leftShoulder = shoulder;
            leftElbow = elbow;
        } else {
            rightShoulder = shoulder;
            rightElbow = elbow;
        }
    }

    void setFrontLeg(int facingDir, float hip, float knee) {
        if (facingDir > 0) {
            rightHip = hip;
            rightKnee = knee;
        } else {
            leftHip = hip;
            leftKnee = knee;
        }
    }

    void setRearLeg(int facingDir, float hip, float knee) {
        if (facingDir > 0) {
            leftHip = hip;
            leftKnee = knee;
        } else {
            rightHip = hip;
            rightKnee = knee;
        }
    }

    static RagdollPose makeIdleGuard(int facingDir, float breathePhase);
    static RagdollPose makeHighGuard(int facingDir);
    static RagdollPose makeLowGuard(int facingDir);
    static RagdollPose makeWalk(int facingDir, float phase, bool forward);
    static RagdollPose makeDash(int facingDir, bool forward);
    static RagdollPose makeJab(int facingDir, float progress);
    static RagdollPose makeCross(int facingDir, float progress);
    static RagdollPose makeOneTwo(int facingDir, float progress);
    static RagdollPose makeMidKick(int facingDir, float progress);
    static RagdollPose makeEWGF(int facingDir, float progress);
    static RagdollPose makeHellSweep(int facingDir, float progress);
    static RagdollPose makeRoundhouse(int facingDir, float progress);
    static RagdollPose makeHopkick(int facingDir, float progress);
    static RagdollPose makeDropkick(int facingDir, float progress);
    static RagdollPose makePowerCrush(int facingDir, float progress);
    static RagdollPose makeThrow(int facingDir, float progress);
    static RagdollPose makeRageArt(int facingDir, float progress);
    static RagdollPose makeHitStun(int facingDir);
    static RagdollPose makeAirJuggle(float airborneTime);
    static RagdollPose makeKnockedDown();
    static RagdollPose makeTechRoll(float progress);
};

class ActiveRagdollController {
public:
    ActiveRagdollController(RagdollSkeleton* skeleton);

    void update(float dt);

    // Movement & Combat Inputs
    void setMoveInput(float moveX, float moveY); // moveX: -1 to 1, moveY: -1 (up) to 1 (down)
    void triggerDash(int dir);
    void triggerMove(MoveId move);
    void jump();

    // Damage & Reactions
    HitResult takeHit(float damage, AttackHeight height, b2Vec2 impulse, bool isLauncher, bool isTrip, bool isThrow = false);
    void popUpJuggle(b2Vec2 impulse);

    // Ground info for suspension
    void setFloorY(float floorY) { m_floorY = floorY; }

    // Ragdoll state overrides
    void setLimp(bool limp);
    bool isLimp() const { return m_actionState == FighterActionState::Limp; }
    void toggleLimp() { setLimp(!isLimp()); }

    // Face direction: 1 = facing right, -1 = facing left
    void setFacingDirection(int dir);
    int getFacingDirection() const { return m_facingDir; }

    // Query states
    FighterActionState getActionState() const { return m_actionState; }
    const MoveDefinition& getCurrentMove() const { return m_currentMove; }
    bool isAttacking() const { return m_actionState == FighterActionState::Attacking; }
    bool isAttackActive() const;
    bool isAttackStartup() const;
    bool isGuarding() const;
    bool isInvincible() const;
    bool isGrounded() const;
    float getMoveProgress() const;
    bool hasHitRegistered() const { return m_hitRegistered; }
    void setHitRegistered(bool registered) { m_hitRegistered = registered; }

    // Contact tips
    sf::Vector2f getActiveStrikingTipPixels() const;

    RagdollSkeleton* getSkeleton() { return m_skeleton; }

private:
    RagdollSkeleton* m_skeleton{ nullptr };

    FighterActionState m_actionState{ FighterActionState::Neutral };
    MoveDefinition m_currentMove;
    int m_facingDir{ 1 };
    float m_moveInputX{ 0.0f };
    float m_moveInputY{ 0.0f };

    float m_floorY{ 800.0f };
    float m_standingHeight{ 55.0f };

    // Timers
    float m_stateTimer{ 0.0f };
    float m_moveTimer{ 0.0f };
    float m_dashTimer{ 0.0f };
    int m_dashDir{ 1 };
    bool m_hitRegistered{ false };
    float m_breathePhase{ 0.0f };
    float m_walkPhase{ 0.0f };
    float m_knockdownTimer{ 0.0f };
    float m_airborneTime{ 0.0f };

    // Poses
    RagdollPose m_currentPose;
    RagdollPose m_targetPose;
    float m_poseBlendSpeed{ 36.0f };

    void evaluateTargetPose();
    void blendPoses(float dt);
    void applyJointDrives();
    void applyLocomotionAndSuspension(float dt);
    void applyUprightStabilizer();
};

} // namespace RagdollEngine
