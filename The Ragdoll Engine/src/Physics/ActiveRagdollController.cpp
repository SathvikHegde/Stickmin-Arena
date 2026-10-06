#include "RagdollEngine/Physics/ActiveRagdollController.hpp"
#include "RagdollEngine/Physics/PhysicsUnits.hpp"
#include <cmath>
#include <algorithm>

namespace RagdollEngine {

// =========================================================================
// RAGDOLL POSES GENERATOR
// =========================================================================

RagdollPose RagdollPose::makeIdleGuard(int facingDir, float breathePhase) {
    RagdollPose p;

    // Spine and neck straight and tall with confident upright posture
    p.neck = 0.0f;
    p.spine = 0.0f;

    // FISTS UP! Martial arts boxing guard:
    // Front arm: tucked close to body guarding chin
    p.setFrontArm(facingDir, -18.0f * facingDir, -80.0f * facingDir);

    // Rear arm: tucked against torso guarding cheek
    p.setRearArm(facingDir, -8.0f * facingDir, -88.0f * facingDir);

    // SOLID CENTERED ATHLETIC BASE:
    // Symmetrical foot stance directly under hips to eliminate any horizontal drift
    p.setFrontLeg(facingDir, 0.0f, 4.0f * facingDir);
    p.setRearLeg(facingDir, 0.0f, 4.0f * facingDir);

    return p;
}

RagdollPose RagdollPose::makeHighGuard(int facingDir) {
    RagdollPose p;
    p.neck = 0.0f;
    p.spine = 0.0f;

    // High crossed guard protecting face
    p.setFrontArm(facingDir, -50.0f * facingDir, -105.0f * facingDir);
    p.setRearArm(facingDir, -35.0f * facingDir, -115.0f * facingDir);

    p.setFrontLeg(facingDir, 0.0f, 4.0f * facingDir);
    p.setRearLeg(facingDir, 0.0f, 4.0f * facingDir);
    return p;
}

RagdollPose RagdollPose::makeLowGuard(int facingDir) {
    RagdollPose p;
    p.neck = 4.0f;
    p.spine = 8.0f * facingDir;

    // Deep crouch with low arm cover
    p.setFrontArm(facingDir, -18.0f * facingDir, -35.0f * facingDir);
    p.setRearArm(facingDir, -25.0f * facingDir, -80.0f * facingDir);

    // Symmetrical crouch leg stance
    p.setFrontLeg(facingDir, 8.0f * facingDir, 32.0f * facingDir);
    p.setRearLeg(facingDir, -8.0f * facingDir, 32.0f * facingDir);
    return p;
}

RagdollPose RagdollPose::makeWalk(int facingDir, float phase, bool forward) {
    RagdollPose p;
    float s = std::sin(phase);
    float c = std::cos(phase);

    // Spine and neck straight with slight dynamic counter-lean against movement inertia
    p.neck = 0.0f;
    p.spine = forward ? (-2.5f * facingDir) : (1.5f * facingDir);

    float dirMult = forward ? 1.0f : -1.0f;

    // Fluid athletic martial arts stride
    float stride = s * 16.0f * dirMult;

    // Front leg: swings forward and lifts knee on upswing to clear the ground
    float frontHip = (3.0f - stride) * facingDir;
    float frontKnee = (s * dirMult > 0.0f ? (10.0f + s * dirMult * 20.0f) : 5.0f) * facingDir;

    // Rear leg: opposite phase with ground-clearance knee lift
    float rearHip = (-2.0f + stride) * facingDir;
    float rearKnee = (s * dirMult < 0.0f ? (10.0f - s * dirMult * 20.0f) : 5.0f) * facingDir;

    p.setFrontLeg(facingDir, frontHip, frontKnee);
    p.setRearLeg(facingDir, rearHip, rearKnee);

    // Martial arts guard maintained while stepping with subtle athletic arm sway
    float armSway = c * 3.5f;
    p.setFrontArm(facingDir, (-18.0f + armSway) * facingDir, -80.0f * facingDir);
    p.setRearArm(facingDir, (-8.0f - armSway) * facingDir, -88.0f * facingDir);

    return p;
}

RagdollPose RagdollPose::makeDash(int facingDir, bool forward) {
    RagdollPose p;

    // Upright stance during dashes
    p.neck = 0.0f;
    p.spine = 0.0f;

    p.setFrontArm(facingDir, -22.0f * facingDir, -82.0f * facingDir);
    p.setRearArm(facingDir, -12.0f * facingDir, -88.0f * facingDir);

    p.setFrontLeg(facingDir, 5.0f * facingDir, 8.0f * facingDir);
    p.setRearLeg(facingDir, -4.0f * facingDir, 8.0f * facingDir);

    return p;
}

RagdollPose RagdollPose::makeJab(int facingDir, float progress) {
    RagdollPose p = makeIdleGuard(facingDir, 0.0f);

    float ext = (progress < 0.35f) ? (progress / 0.35f)
              : (progress < 0.65f) ? 1.0f
              : (1.0f - (progress - 0.65f) / 0.35f);

    p.spine = (5.0f + ext * 12.0f) * facingDir;

    // Front arm extends straight forward
    p.setFrontArm(facingDir, (-28.0f - ext * 58.0f) * facingDir, (-85.0f + ext * 82.0f) * facingDir);
    p.setRearArm(facingDir, -14.0f * facingDir, -100.0f * facingDir);

    p.setFrontLeg(facingDir, -15.0f * facingDir, 18.0f * facingDir);
    p.setRearLeg(facingDir, 14.0f * facingDir, 14.0f * facingDir);
    return p;
}

RagdollPose RagdollPose::makeCross(int facingDir, float progress) {
    RagdollPose p = makeIdleGuard(facingDir, 0.0f);

    float ext = (progress < 0.40f) ? (progress / 0.40f)
              : (progress < 0.70f) ? 1.0f
              : (1.0f - (progress - 0.70f) / 0.30f);

    p.spine = (4.0f + ext * 20.0f) * facingDir;

    // Rear arm unleashes heavy cross
    p.setRearArm(facingDir, (-14.0f - ext * 72.0f) * facingDir, (-95.0f + ext * 90.0f) * facingDir);
    p.setFrontArm(facingDir, -20.0f * facingDir, -100.0f * facingDir);

    p.setFrontLeg(facingDir, -18.0f * facingDir, 22.0f * facingDir);
    p.setRearLeg(facingDir, (12.0f - ext * 8.0f) * facingDir, 16.0f * facingDir);
    return p;
}

RagdollPose RagdollPose::makeOneTwo(int facingDir, float progress) {
    RagdollPose p = makeIdleGuard(facingDir, 0.0f);
    if (progress < 0.45f) {
        // 1: Rapid front jab
        float ext = (progress < 0.22f) ? (progress / 0.22f) : (1.0f - (progress - 0.22f) / 0.23f);
        p.spine = (ext * 8.0f) * facingDir;
        p.setFrontArm(facingDir, (-25.0f - ext * 60.0f) * facingDir, (-85.0f + ext * 80.0f) * facingDir);
        p.setRearArm(facingDir, -10.0f * facingDir, -95.0f * facingDir);
    } else {
        // 2: Instant follow-up rear straight cross
        float t = (progress - 0.45f) / 0.55f;
        float ext = (t < 0.40f) ? (t / 0.40f) : (1.0f - (t - 0.40f) / 0.60f);
        p.spine = (ext * 16.0f) * facingDir;
        p.setFrontArm(facingDir, -18.0f * facingDir, -90.0f * facingDir);
        p.setRearArm(facingDir, (-12.0f - ext * 75.0f) * facingDir, (-95.0f + ext * 90.0f) * facingDir);
    }
    return p;
}

RagdollPose RagdollPose::makeMidKick(int facingDir, float progress) {
    RagdollPose p = makeIdleGuard(facingDir, 0.0f);
    float ext = (progress < 0.35f) ? (progress / 0.35f)
              : (progress < 0.65f) ? 1.0f
              : (1.0f - (progress - 0.65f) / 0.35f);

    p.spine = -6.0f * facingDir;
    p.setFrontLeg(facingDir, (-ext * 72.0f) * facingDir, (ext * 12.0f) * facingDir);
    p.setRearLeg(facingDir, 10.0f * facingDir, -8.0f * facingDir);
    p.setFrontArm(facingDir, -15.0f * facingDir, -75.0f * facingDir);
    p.setRearArm(facingDir, -10.0f * facingDir, -85.0f * facingDir);
    return p;
}

RagdollPose RagdollPose::makeEWGF(int facingDir, float progress) {
    RagdollPose p = makeIdleGuard(facingDir, 0.0f);

    if (progress < 0.30f) {
        float t = progress / 0.30f;
        p.spine = 22.0f * facingDir;
        p.setFrontArm(facingDir, -15.0f * facingDir, -95.0f * facingDir);
        p.setRearArm(facingDir, -10.0f * facingDir, -95.0f * facingDir);
        p.setFrontLeg(facingDir, -28.0f * facingDir * t, 48.0f * facingDir * t);
        p.setRearLeg(facingDir, 22.0f * facingDir * t, 42.0f * facingDir * t);
    } else if (progress < 0.70f) {
        // Explosive rising electric uppercut!
        p.neck = -12.0f * facingDir;
        p.spine = -14.0f * facingDir;
        p.setFrontArm(facingDir, -135.0f * facingDir, -20.0f * facingDir);
        p.setRearArm(facingDir, 20.0f * facingDir, -85.0f * facingDir);
        p.setFrontLeg(facingDir, -16.0f * facingDir, 18.0f * facingDir);
        p.setRearLeg(facingDir, 12.0f * facingDir, 16.0f * facingDir);
    } else {
        p = makeIdleGuard(facingDir, 0.0f);
    }
    return p;
}

RagdollPose RagdollPose::makeHellSweep(int facingDir, float progress) {
    RagdollPose p;
    float ext = (progress < 0.35f) ? (progress / 0.35f)
              : (progress < 0.70f) ? 1.0f
              : (1.0f - (progress - 0.70f) / 0.30f);

    p.neck = 10.0f;
    p.spine = 20.0f * facingDir;

    // Rear leg sweeps straight forward along floor
    p.setRearLeg(facingDir, (-ext * 85.0f) * facingDir, 6.0f * facingDir);
    p.setFrontLeg(facingDir, 32.0f * facingDir, 58.0f * facingDir);

    p.setFrontArm(facingDir, -20.0f * facingDir, -85.0f * facingDir);
    p.setRearArm(facingDir, -15.0f * facingDir, -85.0f * facingDir);

    return p;
}

RagdollPose RagdollPose::makeRoundhouse(int facingDir, float progress) {
    RagdollPose p = makeIdleGuard(facingDir, 0.0f);

    float ext = (progress < 0.35f) ? (progress / 0.35f)
              : (progress < 0.70f) ? 1.0f
              : (1.0f - (progress - 0.70f) / 0.30f);

    p.spine = -18.0f * facingDir;

    // Front leg executes high whip kick
    p.setFrontLeg(facingDir, (-ext * 95.0f) * facingDir, ((1.0f - ext) * 55.0f + ext * 6.0f) * facingDir);
    p.setRearLeg(facingDir, 14.0f * facingDir, 16.0f * facingDir);

    p.setFrontArm(facingDir, 25.0f * facingDir, -65.0f * facingDir);
    p.setRearArm(facingDir, -30.0f * facingDir, -95.0f * facingDir);

    return p;
}

RagdollPose RagdollPose::makeHopkick(int facingDir, float progress) {
    RagdollPose p;
    float ext = (progress < 0.35f) ? (progress / 0.35f)
              : (progress < 0.65f) ? 1.0f
              : (1.0f - (progress - 0.65f) / 0.35f);

    p.neck = -6.0f * facingDir;
    p.spine = -10.0f * facingDir;

    // Front leg snaps upwards into chin
    p.setFrontLeg(facingDir, (-ext * 92.0f) * facingDir, 8.0f * facingDir);
    p.setRearLeg(facingDir, 22.0f * facingDir, 45.0f * facingDir);

    p.setFrontArm(facingDir, -40.0f * facingDir, -60.0f * facingDir);
    p.setRearArm(facingDir, 30.0f * facingDir, -70.0f * facingDir);

    return p;
}

RagdollPose RagdollPose::makeDropkick(int facingDir, float progress) {
    RagdollPose p;
    p.neck = 10.0f;
    p.spine = 15.0f * facingDir;

    p.leftHip = -75.0f * facingDir;
    p.leftKnee = 8.0f * facingDir;
    p.rightHip = -70.0f * facingDir;
    p.rightKnee = 8.0f * facingDir;

    p.setFrontArm(facingDir, -60.0f * facingDir, -40.0f * facingDir);
    p.setRearArm(facingDir, 40.0f * facingDir, -40.0f * facingDir);

    return p;
}

RagdollPose RagdollPose::makePowerCrush(int facingDir, float progress) {
    RagdollPose p;
    if (progress < 0.45f) {
        // Armored wind-up stance
        float t = progress / 0.45f;
        p.neck = -4.0f * facingDir;
        p.spine = -12.0f * facingDir * t;
        p.setFrontArm(facingDir, -42.0f * facingDir, -115.0f * facingDir);
        p.setRearArm(facingDir, -38.0f * facingDir, -115.0f * facingDir);
        p.setFrontLeg(facingDir, -16.0f * facingDir, 24.0f * facingDir);
        p.setRearLeg(facingDir, 18.0f * facingDir, 20.0f * facingDir);
    } else {
        // Devastating armored lunging chest blow!
        float t = (progress - 0.45f) / 0.55f;
        float ext = (t < 0.35f) ? (t / 0.35f) : (1.0f - (t - 0.35f) / 0.65f);
        p.neck = 4.0f * facingDir;
        p.spine = (24.0f * ext) * facingDir;
        p.setFrontArm(facingDir, (-42.0f - ext * 45.0f) * facingDir, (-115.0f + ext * 105.0f) * facingDir);
        p.setRearArm(facingDir, (-38.0f - ext * 45.0f) * facingDir, (-115.0f + ext * 105.0f) * facingDir);
        p.setFrontLeg(facingDir, (-16.0f - ext * 14.0f) * facingDir, 20.0f * facingDir);
        p.setRearLeg(facingDir, (18.0f + ext * 12.0f) * facingDir, 15.0f * facingDir);
    }
    return p;
}

RagdollPose RagdollPose::makeThrow(int facingDir, float progress) {
    RagdollPose p;
    if (progress < 0.45f) {
        // Outstretched reach for opponent's collar
        float t = progress / 0.45f;
        p.neck = 4.0f * facingDir;
        p.spine = (14.0f * t) * facingDir;
        p.setFrontArm(facingDir, (-20.0f - t * 60.0f) * facingDir, (-80.0f + t * 65.0f) * facingDir);
        p.setRearArm(facingDir, (-10.0f - t * 65.0f) * facingDir, (-85.0f + t * 70.0f) * facingDir);
        p.setFrontLeg(facingDir, -12.0f * facingDir, 16.0f * facingDir);
        p.setRearLeg(facingDir, 12.0f * facingDir, 14.0f * facingDir);
    } else {
        // Violent downward judo slam!
        float t = (progress - 0.45f) / 0.55f;
        p.neck = 8.0f * facingDir;
        p.spine = (14.0f + t * 24.0f) * facingDir;
        p.setFrontArm(facingDir, (35.0f * t) * facingDir, (-20.0f - t * 40.0f) * facingDir);
        p.setRearArm(facingDir, (40.0f * t) * facingDir, (-20.0f - t * 40.0f) * facingDir);
        p.setFrontLeg(facingDir, -20.0f * facingDir, 28.0f * facingDir);
        p.setRearLeg(facingDir, 18.0f * facingDir, 22.0f * facingDir);
    }
    return p;
}

RagdollPose RagdollPose::makeRageArt(int facingDir, float progress) {
    RagdollPose p;
    if (progress < 0.30f) {
        // Cinematic charging stance: fists drawn back, eyes glowing
        float t = progress / 0.30f;
        p.neck = -8.0f * facingDir;
        p.spine = (-16.0f * t) * facingDir;
        p.setFrontArm(facingDir, (25.0f * t) * facingDir, -95.0f * facingDir);
        p.setRearArm(facingDir, (30.0f * t) * facingDir, -100.0f * facingDir);
        p.setFrontLeg(facingDir, -20.0f * facingDir * t, 30.0f * facingDir * t);
        p.setRearLeg(facingDir, 20.0f * facingDir * t, 25.0f * facingDir * t);
    } else if (progress < 0.70f) {
        // Supersonic armor thrust punch
        p.neck = 4.0f * facingDir;
        p.spine = 22.0f * facingDir;
        p.setFrontArm(facingDir, 25.0f * facingDir, -85.0f * facingDir);
        p.setRearArm(facingDir, -100.0f * facingDir, 0.0f);
        p.setFrontLeg(facingDir, -24.0f * facingDir, 22.0f * facingDir);
        p.setRearLeg(facingDir, 28.0f * facingDir, 16.0f * facingDir);
    } else {
        p = makeIdleGuard(facingDir, 0.0f);
    }
    return p;
}

RagdollPose RagdollPose::makeHitStun(int facingDir) {
    RagdollPose p;
    p.neck = -16.0f * facingDir;
    p.spine = -20.0f * facingDir; // Reeling backward

    p.setFrontArm(facingDir, 35.0f * facingDir, -40.0f * facingDir);
    p.setRearArm(facingDir, 25.0f * facingDir, -45.0f * facingDir);

    p.setFrontLeg(facingDir, -8.0f * facingDir, 22.0f * facingDir);
    p.setRearLeg(facingDir, 18.0f * facingDir, 18.0f * facingDir);

    return p;
}

RagdollPose RagdollPose::makeAirJuggle(float airborneTime) {
    RagdollPose p;
    float s = std::sin(airborneTime * 8.0f);

    p.neck = s * 10.0f;
    p.spine = s * 12.0f;

    p.leftShoulder = 30.0f + s * 25.0f;
    p.leftElbow = -35.0f;
    p.rightShoulder = -30.0f - s * 25.0f;
    p.rightElbow = -35.0f;

    p.leftHip = -20.0f + s * 15.0f;
    p.leftKnee = 30.0f;
    p.rightHip = 20.0f - s * 15.0f;
    p.rightKnee = 30.0f;

    return p;
}

RagdollPose RagdollPose::makeKnockedDown() {
    RagdollPose p;
    p.neck = 0.0f;
    p.spine = 0.0f;
    p.leftShoulder = 20.0f;
    p.leftElbow = -15.0f;
    p.rightShoulder = -20.0f;
    p.rightElbow = -15.0f;
    p.leftHip = 10.0f;
    p.leftKnee = 15.0f;
    p.rightHip = -10.0f;
    p.rightKnee = 15.0f;
    return p;
}

RagdollPose RagdollPose::makeTechRoll(float progress) {
    RagdollPose p;
    p.spine = 15.0f;
    p.leftHip = -50.0f * (1.0f - progress);
    p.leftKnee = 70.0f * (1.0f - progress);
    p.rightHip = 50.0f * (1.0f - progress);
    p.rightKnee = 70.0f * (1.0f - progress);

    p.leftShoulder = 35.0f;
    p.leftElbow = -60.0f;
    p.rightShoulder = -35.0f;
    p.rightElbow = -60.0f;

    return p;
}

// =========================================================================
// ACTIVE RAGDOLL CONTROLLER IMPLEMENTATION
// =========================================================================

ActiveRagdollController::ActiveRagdollController(RagdollSkeleton* skeleton)
    : m_skeleton(skeleton) {
    m_currentPose = RagdollPose::makeIdleGuard(1, 0.0f);
    m_targetPose = m_currentPose;
}

void ActiveRagdollController::setFacingDirection(int dir) {
    if (dir != 0) {
        m_facingDir = (dir > 0) ? 1 : -1;
    }
}

void ActiveRagdollController::setMoveInput(float moveX, float moveY) {
    m_moveInputX = std::clamp(moveX, -1.0f, 1.0f);
    m_moveInputY = std::clamp(moveY, -1.0f, 1.0f);

    if (m_actionState == FighterActionState::Neutral ||
        m_actionState == FighterActionState::MovingForward ||
        m_actionState == FighterActionState::MovingBackward ||
        m_actionState == FighterActionState::Crouching ||
        m_actionState == FighterActionState::HighGuarding ||
        m_actionState == FighterActionState::LowGuarding) {

        if (m_moveInputY > 0.4f) {
            // Holding Down
            if (m_moveInputX * m_facingDir < -0.2f) {
                m_actionState = FighterActionState::LowGuarding; // Down+Back = Crouch Block
            } else {
                m_actionState = FighterActionState::Crouching;
            }
        } else if (m_moveInputX * m_facingDir < -0.2f) {
            // Holding Back away from opponent = Walk Backward & High Guard (Tekken style!)
            m_actionState = FighterActionState::MovingBackward;
        } else if (m_moveInputX * m_facingDir > 0.2f) {
            m_actionState = FighterActionState::MovingForward;
        } else {
            m_actionState = FighterActionState::Neutral;
        }
    }
}

void ActiveRagdollController::triggerDash(int dir) {
    if (m_actionState == FighterActionState::HitStun ||
        m_actionState == FighterActionState::LaunchedJuggle ||
        m_actionState == FighterActionState::KnockedDown ||
        m_actionState == FighterActionState::GettingUp ||
        m_actionState == FighterActionState::Limp) {
        return;
    }

    m_actionState = FighterActionState::Dashing;
    m_dashDir = (dir > 0) ? 1 : -1;
    m_dashTimer = 0.20f;

    b2BodyId hips = m_skeleton->getHips();
    b2BodyId torso = m_skeleton->getTorso();
    if (b2Body_IsValid(hips)) {
        float speed = (m_dashDir == m_facingDir) ? 10.0f : -7.5f;
        b2Vec2 imp = { speed * m_facingDir * 0.5f, 0.0f }; // Purely horizontal impulse
        b2Body_ApplyLinearImpulseToCenter(hips, imp, true);
        if (b2Body_IsValid(torso)) {
            b2Body_ApplyLinearImpulseToCenter(torso, imp, true);
        }
    }
}

void ActiveRagdollController::jump() {
    if (!isGrounded() || !m_skeleton) return;
    if (m_actionState == FighterActionState::HitStun ||
        m_actionState == FighterActionState::LaunchedJuggle ||
        m_actionState == FighterActionState::KnockedDown ||
        m_actionState == FighterActionState::GettingUp ||
        m_actionState == FighterActionState::Limp) {
        return;
    }

    b2BodyId hips = m_skeleton->getHips();
    b2BodyId torso = m_skeleton->getTorso();
    if (b2Body_IsValid(hips) && b2Body_IsValid(torso)) {
        b2Vec2 jumpImp = { m_moveInputX * 2.5f, -9.5f };
        b2Body_ApplyLinearImpulseToCenter(hips, jumpImp, true);
        b2Body_ApplyLinearImpulseToCenter(torso, jumpImp, true);
    }
}

void ActiveRagdollController::triggerMove(MoveId move) {
    if (m_actionState == FighterActionState::HitStun ||
        m_actionState == FighterActionState::LaunchedJuggle ||
        m_actionState == FighterActionState::KnockedDown ||
        m_actionState == FighterActionState::GettingUp ||
        m_actionState == FighterActionState::Limp) {
        return;
    }

    // Configure move definition
    MoveDefinition def;
    def.id = move;

    switch (move) {
    case MoveId::FlashJab:
        def.name = "Flash Jab";
        def.height = AttackHeight::High;
        def.startup = 0.06f;
        def.active = 0.05f;
        def.recovery = 0.08f;
        def.damage = 9.0f;
        def.launchImpulse = { m_facingDir * 6.0f, -2.5f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::RightForearm : LimbType::LeftForearm;
        break;

    case MoveId::StraightCross:
        def.name = "Straight Cross";
        def.height = AttackHeight::Mid;
        def.startup = 0.08f;
        def.active = 0.06f;
        def.recovery = 0.10f;
        def.damage = 15.0f;
        def.launchImpulse = { m_facingDir * 11.0f, -4.0f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::LeftForearm : LimbType::RightForearm;
        break;

    case MoveId::ElectricWindGodFist:
        def.name = "Electric Wind God Fist";
        def.height = AttackHeight::Mid;
        def.startup = 0.12f;
        def.active = 0.08f;
        def.recovery = 0.14f;
        def.damage = 25.0f;
        def.isLauncher = true;
        def.isElectric = true;
        def.launchImpulse = { m_facingDir * 4.0f, -17.5f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::RightForearm : LimbType::LeftForearm;
        break;

    case MoveId::HellSweep:
        def.name = "Hell Sweep";
        def.height = AttackHeight::Low;
        def.startup = 0.11f;
        def.active = 0.07f;
        def.recovery = 0.13f;
        def.damage = 16.0f;
        def.isTrip = true;
        def.launchImpulse = { m_facingDir * 8.0f, -4.5f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::RightShin : LimbType::LeftShin;
        break;

    case MoveId::AxeRoundhouse:
        def.name = "Axe Roundhouse";
        def.height = AttackHeight::High;
        def.startup = 0.12f;
        def.active = 0.07f;
        def.recovery = 0.15f;
        def.damage = 22.0f;
        def.launchImpulse = { m_facingDir * 15.0f, -7.5f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::RightShin : LimbType::LeftShin;
        break;

    case MoveId::Hopkick:
        def.name = "Hopkick";
        def.height = AttackHeight::Mid;
        def.startup = 0.10f;
        def.active = 0.07f;
        def.recovery = 0.16f;
        def.damage = 18.0f;
        def.isLauncher = true;
        def.launchImpulse = { m_facingDir * 4.5f, -15.5f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::RightShin : LimbType::LeftShin;
        break;

    case MoveId::FlyingDropkick:
        def.name = "Flying Dropkick";
        def.height = AttackHeight::Mid;
        def.startup = 0.14f;
        def.active = 0.12f;
        def.recovery = 0.20f;
        def.damage = 28.0f;
        def.launchImpulse = { m_facingDir * 20.0f, -6.5f };
        def.strikingLimb = LimbType::RightShin;
        break;

    case MoveId::OneTwoString:
        def.name = "1, 2 String";
        def.height = AttackHeight::High;
        def.startup = 0.07f;
        def.active = 0.09f;
        def.recovery = 0.11f;
        def.damage = 17.0f;
        def.launchImpulse = { m_facingDir * 9.0f, -3.0f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::LeftForearm : LimbType::RightForearm;
        break;

    case MoveId::MidKick:
        def.name = "Mid Kick";
        def.height = AttackHeight::Mid;
        def.startup = 0.09f;
        def.active = 0.06f;
        def.recovery = 0.11f;
        def.damage = 14.0f;
        def.launchImpulse = { m_facingDir * 7.5f, -3.5f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::RightShin : LimbType::LeftShin;
        break;

    case MoveId::PowerCrush:
        def.name = "Power Crush";
        def.height = AttackHeight::Mid;
        def.startup = 0.15f;
        def.active = 0.08f;
        def.recovery = 0.17f;
        def.damage = 24.0f;
        def.isPowerCrush = true;
        def.launchImpulse = { m_facingDir * 18.0f, -5.5f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::RightForearm : LimbType::LeftForearm;
        break;

    case MoveId::Throw:
        def.name = "Command Throw";
        def.height = AttackHeight::High;
        def.startup = 0.12f;
        def.active = 0.08f;
        def.recovery = 0.16f;
        def.damage = 26.0f;
        def.isThrow = true;
        def.isTrip = true;
        def.launchImpulse = { m_facingDir * 12.0f, -4.5f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::RightForearm : LimbType::LeftForearm;
        break;

    case MoveId::RageArt:
        def.name = "Rage Art";
        def.height = AttackHeight::Mid;
        def.startup = 0.18f;
        def.active = 0.12f;
        def.recovery = 0.22f;
        def.damage = 48.0f;
        def.isRageArt = true;
        def.isPowerCrush = true;
        def.isLauncher = true;
        def.launchImpulse = { m_facingDir * 24.0f, -14.0f };
        def.strikingLimb = (m_facingDir > 0) ? LimbType::LeftForearm : LimbType::RightForearm;
        break;

    default:
        return;
    }

    m_currentMove = def;
    m_actionState = FighterActionState::Attacking;
    m_moveTimer = 0.0f;
    m_hitRegistered = false;

    // Forward attack step impulse
    b2BodyId torso = m_skeleton->getTorso();
    if (b2Body_IsValid(torso)) {
        float fwdPush = (move == MoveId::ElectricWindGodFist) ? 6.0f
                      : (move == MoveId::FlyingDropkick) ? 12.0f
                      : (move == MoveId::PowerCrush) ? 7.0f
                      : (move == MoveId::RageArt) ? 10.0f
                      : (move == MoveId::Hopkick) ? 4.0f
                      : 2.5f;
        float upPush = (move == MoveId::Hopkick) ? -6.5f
                     : (move == MoveId::FlyingDropkick) ? -4.0f
                     : 0.0f;
        b2Body_ApplyLinearImpulseToCenter(torso, b2Vec2{ fwdPush * m_facingDir, upPush }, true);
    }
}

HitResult ActiveRagdollController::takeHit(float damage, AttackHeight height, b2Vec2 impulse, bool isLauncher, bool isTrip, bool isThrow) {
    if (isInvincible()) return HitResult::Miss;

    // 1. Ducking under High Attacks or Throws
    if (m_actionState == FighterActionState::LowGuarding || m_actionState == FighterActionState::Crouching) {
        if (height == AttackHeight::High || isThrow) {
            // Ducked completely under high strikes and high throws!
            return HitResult::Miss;
        }
    }

    // 2. Command Throw Resolution (Unblockable vs standing guard, broken if defender attacks during startup)
    if (isThrow) {
        if (isAttackStartup()) {
            return HitResult::ThrowBroken;
        }
        m_actionState = FighterActionState::KnockedDown;
        m_knockdownTimer = 0.0f;
        b2BodyId hips = m_skeleton->getHips();
        if (b2Body_IsValid(hips)) {
            b2Body_ApplyLinearImpulseToCenter(hips, impulse, true);
        }
        return HitResult::ThrowGrabbed;
    }

    // 3. Power Crush Armor (Absorbs High and Mid attacks without interruption!)
    if (m_actionState == FighterActionState::Attacking && m_currentMove.isPowerCrush) {
        if (height == AttackHeight::High || height == AttackHeight::Mid) {
            // Absorb hit through armor! Slight pushback, but no hitstun or launch!
            b2BodyId torso = m_skeleton->getTorso();
            if (b2Body_IsValid(torso)) {
                b2Body_ApplyLinearImpulseToCenter(torso, b2Vec2{ impulse.x * 0.25f, 0.0f }, true);
            }
            return HitResult::PowerCrushAbsorb;
        }
        // If it's a Low attack, Power Crush gets interrupted/crushed!
    }

    // 4. Guard / Blocking logic (Authentic Tekken: Active Guard, Back-Walk Guard & Neutral Guard)
    if (m_actionState == FighterActionState::HighGuarding ||
        m_actionState == FighterActionState::MovingBackward ||
        m_actionState == FighterActionState::Neutral) {
        // High Guard, Back-Walk & Neutral Guard block High and Mid attacks!
        if (height == AttackHeight::High || height == AttackHeight::Mid) {
            b2BodyId torso = m_skeleton->getTorso();
            if (b2Body_IsValid(torso)) {
                b2Body_ApplyLinearImpulseToCenter(torso, b2Vec2{ impulse.x * 0.35f, 0.0f }, true);
            }
            return HitResult::Blocked;
        }
        // Low attacks pierce High/Neutral Guard!
    } else if (m_actionState == FighterActionState::LowGuarding) {
        // Low Guard blocks Low attacks
        if (height == AttackHeight::Low) {
            b2BodyId torso = m_skeleton->getTorso();
            if (b2Body_IsValid(torso)) {
                b2Body_ApplyLinearImpulseToCenter(torso, b2Vec2{ impulse.x * 0.35f, 0.0f }, true);
            }
            return HitResult::Blocked;
        }
        // Mid attacks crush Low Guard!
    }

    // 5. Was it a counter-hit? (Attacking during startup)
    bool isCounter = isAttackStartup();

    b2BodyId hips = m_skeleton->getHips();
    b2BodyId torso = m_skeleton->getTorso();

    if (isLauncher) {
        m_actionState = FighterActionState::LaunchedJuggle;
        m_airborneTime = 0.0f;

        // Reduce spring stiffness for dramatic launch splay
        for (size_t i = 0; i < static_cast<size_t>(JointType::Count); ++i) {
            b2JointId joint = m_skeleton->getJoint(static_cast<JointType>(i));
            if (b2Joint_IsValid(joint)) {
                b2RevoluteJoint_SetSpringHertz(joint, 6.0f);
            }
        }

        if (b2Body_IsValid(hips)) b2Body_ApplyLinearImpulseToCenter(hips, impulse, true);
        if (b2Body_IsValid(torso)) b2Body_ApplyLinearImpulseToCenter(torso, impulse, true);
    } else if (isTrip) {
        m_actionState = FighterActionState::KnockedDown;
        m_knockdownTimer = 0.0f;
        if (b2Body_IsValid(hips)) b2Body_ApplyLinearImpulseToCenter(hips, impulse, true);
    } else {
        // Normal hit stun
        m_actionState = FighterActionState::HitStun;
        m_stateTimer = 0.20f;
        if (b2Body_IsValid(torso)) b2Body_ApplyLinearImpulseToCenter(torso, impulse, true);
    }

    return isCounter ? HitResult::CounterHit : HitResult::CleanHit;
}

void ActiveRagdollController::popUpJuggle(b2Vec2 impulse) {
    if (m_actionState == FighterActionState::LaunchedJuggle) {
        b2BodyId hips = m_skeleton->getHips();
        b2BodyId torso = m_skeleton->getTorso();
        if (b2Body_IsValid(hips)) b2Body_ApplyLinearImpulseToCenter(hips, impulse, true);
        if (b2Body_IsValid(torso)) b2Body_ApplyLinearImpulseToCenter(torso, impulse, true);
    }
}

void ActiveRagdollController::setLimp(bool limp) {
    m_actionState = limp ? FighterActionState::Limp : FighterActionState::Neutral;
    if (!m_skeleton) return;

    for (size_t i = 0; i < static_cast<size_t>(JointType::Count); ++i) {
        b2JointId joint = m_skeleton->getJoint(static_cast<JointType>(i));
        if (b2Joint_IsValid(joint)) {
            b2RevoluteJoint_EnableSpring(joint, !limp);
            b2RevoluteJoint_EnableLimit(joint, !limp);
            b2RevoluteJoint_SetSpringHertz(joint, limp ? 0.0f : 14.0f);
        }
    }
}

bool ActiveRagdollController::isAttackActive() const {
    if (m_actionState != FighterActionState::Attacking) return false;
    return (m_moveTimer >= m_currentMove.startup &&
            m_moveTimer < (m_currentMove.startup + m_currentMove.active));
}

bool ActiveRagdollController::isAttackStartup() const {
    if (m_actionState != FighterActionState::Attacking) return false;
    return (m_moveTimer < m_currentMove.startup);
}

bool ActiveRagdollController::isGuarding() const {
    return (m_actionState == FighterActionState::HighGuarding ||
            m_actionState == FighterActionState::MovingBackward ||
            m_actionState == FighterActionState::LowGuarding);
}

bool ActiveRagdollController::isInvincible() const {
    return (m_actionState == FighterActionState::GettingUp);
}

bool ActiveRagdollController::isGrounded() const {
    if (!m_skeleton) return false;
    b2BodyId hips = m_skeleton->getHips();
    if (!b2Body_IsValid(hips)) return false;
    float currentY = PhysicsUnits::toPixels(b2Body_GetPosition(hips)).y;
    return (currentY >= (m_floorY - m_standingHeight - 20.0f));
}

float ActiveRagdollController::getMoveProgress() const {
    if (m_actionState != FighterActionState::Attacking) return 0.0f;
    float total = m_currentMove.totalDuration();
    if (total <= 0.001f) return 1.0f;
    return std::clamp(m_moveTimer / total, 0.0f, 1.0f);
}

sf::Vector2f ActiveRagdollController::getActiveStrikingTipPixels() const {
    if (!m_skeleton) return sf::Vector2f(0.0f, 0.0f);

    switch (m_currentMove.strikingLimb) {
    case LimbType::LeftForearm:
        return m_skeleton->getLeftFistPixels();
    case LimbType::RightForearm:
        return m_skeleton->getRightFistPixels();
    case LimbType::LeftShin:
        return m_skeleton->getLeftFootPixels();
    case LimbType::RightShin:
        return m_skeleton->getRightFootPixels();
    default:
        return m_skeleton->getPositionPixels();
    }
}

void ActiveRagdollController::update(float dt) {
    if (!m_skeleton) return;
    if (m_actionState == FighterActionState::Limp) return;

    // 1. Update State Timers & Transitions
    m_breathePhase += dt * 3.5f;

    if (m_actionState == FighterActionState::Attacking) {
        m_moveTimer += dt;
        if (m_moveTimer >= m_currentMove.totalDuration()) {
            m_actionState = FighterActionState::Neutral;
        }
    } else if (m_actionState == FighterActionState::Dashing) {
        m_dashTimer -= dt;
        if (m_dashTimer <= 0.0f) {
            m_actionState = FighterActionState::Neutral;
        }
    } else if (m_actionState == FighterActionState::HitStun) {
        m_stateTimer -= dt;
        if (m_stateTimer <= 0.0f) {
            m_actionState = FighterActionState::Neutral;
        }
    } else if (m_actionState == FighterActionState::LaunchedJuggle) {
        m_airborneTime += dt;
        b2BodyId hips = m_skeleton->getHips();
        if (b2Body_IsValid(hips)) {
            float hipsY = PhysicsUnits::toPixels(b2Body_GetPosition(hips)).y;
            float vy = b2Body_GetLinearVelocity(hips).y;

            // When landing on ground after juggle
            if (hipsY >= (m_floorY - 30.0f) && vy > -1.0f && m_airborneTime > 0.35f) {
                m_actionState = FighterActionState::KnockedDown;
                m_knockdownTimer = 0.0f;
            }
        }
    } else if (m_actionState == FighterActionState::KnockedDown) {
        m_knockdownTimer += dt;
        if (m_knockdownTimer >= 0.45f) {
            // Automatic Tech Roll / Get-Up recovery!
            m_actionState = FighterActionState::GettingUp;
            m_stateTimer = 0.0f;

            // Restore normal joint spring stiffness
            for (size_t i = 0; i < static_cast<size_t>(JointType::Count); ++i) {
                b2JointId joint = m_skeleton->getJoint(static_cast<JointType>(i));
                if (b2Joint_IsValid(joint)) {
                    b2RevoluteJoint_SetSpringHertz(joint, 14.0f);
                }
            }

            b2BodyId hips = m_skeleton->getHips();
            if (b2Body_IsValid(hips)) {
                b2Body_ApplyLinearImpulseToCenter(hips, b2Vec2{ -m_facingDir * 3.5f, -6.0f }, true);
            }
        }
    } else if (m_actionState == FighterActionState::GettingUp) {
        m_stateTimer += dt;
        if (m_stateTimer >= 0.35f) {
            m_actionState = FighterActionState::Neutral;
        }
    } else if (m_actionState == FighterActionState::MovingForward ||
               m_actionState == FighterActionState::MovingBackward) {
        m_walkPhase += dt * 10.0f;
    }

    // 2. Evaluate Target Pose based on State
    evaluateTargetPose();

    // 3. Smooth Exponential Blend of Poses
    blendPoses(dt);

    // 4. Drive Box2D Joint Springs
    applyJointDrives();

    // 5. Ground Suspension & Locomotion Physics
    applyLocomotionAndSuspension(dt);

    // 6. Upright Stabilization
    applyUprightStabilizer();
}

void ActiveRagdollController::evaluateTargetPose() {
    switch (m_actionState) {
    case FighterActionState::Neutral:
        m_targetPose = RagdollPose::makeIdleGuard(m_facingDir, m_breathePhase);
        break;

    case FighterActionState::MovingForward:
        m_targetPose = RagdollPose::makeWalk(m_facingDir, m_walkPhase, true);
        break;

    case FighterActionState::MovingBackward:
        m_targetPose = RagdollPose::makeWalk(m_facingDir, m_walkPhase, false);
        break;

    case FighterActionState::Dashing:
        m_targetPose = RagdollPose::makeDash(m_facingDir, m_dashDir == m_facingDir);
        break;

    case FighterActionState::HighGuarding:
        m_targetPose = RagdollPose::makeHighGuard(m_facingDir);
        break;

    case FighterActionState::LowGuarding:
    case FighterActionState::Crouching:
        m_targetPose = RagdollPose::makeLowGuard(m_facingDir);
        break;

    case FighterActionState::Attacking: {
        float p = getMoveProgress();
        switch (m_currentMove.id) {
        case MoveId::FlashJab:
            m_targetPose = RagdollPose::makeJab(m_facingDir, p);
            break;
        case MoveId::StraightCross:
            m_targetPose = RagdollPose::makeCross(m_facingDir, p);
            break;
        case MoveId::ElectricWindGodFist:
            m_targetPose = RagdollPose::makeEWGF(m_facingDir, p);
            break;
        case MoveId::HellSweep:
            m_targetPose = RagdollPose::makeHellSweep(m_facingDir, p);
            break;
        case MoveId::AxeRoundhouse:
            m_targetPose = RagdollPose::makeRoundhouse(m_facingDir, p);
            break;
        case MoveId::Hopkick:
            m_targetPose = RagdollPose::makeHopkick(m_facingDir, p);
            break;
        case MoveId::FlyingDropkick:
            m_targetPose = RagdollPose::makeDropkick(m_facingDir, p);
            break;
        case MoveId::OneTwoString:
            m_targetPose = RagdollPose::makeOneTwo(m_facingDir, p);
            break;
        case MoveId::MidKick:
            m_targetPose = RagdollPose::makeMidKick(m_facingDir, p);
            break;
        case MoveId::PowerCrush:
            m_targetPose = RagdollPose::makePowerCrush(m_facingDir, p);
            break;
        case MoveId::Throw:
            m_targetPose = RagdollPose::makeThrow(m_facingDir, p);
            break;
        case MoveId::RageArt:
            m_targetPose = RagdollPose::makeRageArt(m_facingDir, p);
            break;
        default:
            m_targetPose = RagdollPose::makeIdleGuard(m_facingDir, m_breathePhase);
            break;
        }
        break;
    }

    case FighterActionState::HitStun:
        m_targetPose = RagdollPose::makeHitStun(m_facingDir);
        break;

    case FighterActionState::LaunchedJuggle:
        m_targetPose = RagdollPose::makeAirJuggle(m_airborneTime);
        break;

    case FighterActionState::KnockedDown:
        m_targetPose = RagdollPose::makeKnockedDown();
        break;

    case FighterActionState::GettingUp:
        m_targetPose = RagdollPose::makeTechRoll(std::clamp(m_stateTimer / 0.35f, 0.0f, 1.0f));
        break;

    default:
        m_targetPose = RagdollPose::makeIdleGuard(m_facingDir, m_breathePhase);
        break;
    }
}

void ActiveRagdollController::blendPoses(float dt) {
    float rate = m_poseBlendSpeed * dt;
    rate = std::clamp(rate, 0.0f, 1.0f);

    auto blend = [rate](float& cur, float target) {
        cur += (target - cur) * rate;
    };

    blend(m_currentPose.neck, m_targetPose.neck);
    blend(m_currentPose.spine, m_targetPose.spine);
    blend(m_currentPose.leftShoulder, m_targetPose.leftShoulder);
    blend(m_currentPose.leftElbow, m_targetPose.leftElbow);
    blend(m_currentPose.rightShoulder, m_targetPose.rightShoulder);
    blend(m_currentPose.rightElbow, m_targetPose.rightElbow);
    blend(m_currentPose.leftHip, m_targetPose.leftHip);
    blend(m_currentPose.leftKnee, m_targetPose.leftKnee);
    blend(m_currentPose.rightHip, m_targetPose.rightHip);
    blend(m_currentPose.rightKnee, m_targetPose.rightKnee);
}

void ActiveRagdollController::applyJointDrives() {
    auto driveJoint = [&](JointType type, float targetAngleDeg) {
        b2JointId joint = m_skeleton->getJoint(type);
        if (!b2Joint_IsValid(joint)) return;
        b2RevoluteJoint_SetTargetAngle(joint, PhysicsUnits::degToRad(targetAngleDeg));
    };

    driveJoint(JointType::Neck, m_currentPose.neck);
    driveJoint(JointType::Spine, m_currentPose.spine);
    driveJoint(JointType::LeftShoulder, m_currentPose.leftShoulder);
    driveJoint(JointType::LeftElbow, m_currentPose.leftElbow);
    driveJoint(JointType::RightShoulder, m_currentPose.rightShoulder);
    driveJoint(JointType::RightElbow, m_currentPose.rightElbow);
    driveJoint(JointType::LeftHip, m_currentPose.leftHip);
    driveJoint(JointType::LeftKnee, m_currentPose.leftKnee);
    driveJoint(JointType::RightHip, m_currentPose.rightHip);
    driveJoint(JointType::RightKnee, m_currentPose.rightKnee);
}

void ActiveRagdollController::applyLocomotionAndSuspension(float dt) {
    b2BodyId hips = m_skeleton->getHips();
    b2BodyId torso = m_skeleton->getTorso();
    if (!b2Body_IsValid(hips)) return;

    // Disabled during ragdoll/juggle/knockdown
    if (m_actionState == FighterActionState::LaunchedJuggle ||
        m_actionState == FighterActionState::KnockedDown) {
        return;
    }

    // 1. Ground Suspension (Virtual Spring holding hips at ideal stance height)
    float currentY = PhysicsUnits::toPixels(b2Body_GetPosition(hips)).y;
    float targetHeight = (m_actionState == FighterActionState::Crouching ||
                          m_actionState == FighterActionState::LowGuarding)
                         ? (m_standingHeight * 0.72f)
                         : m_standingHeight;

    float targetY = m_floorY - targetHeight;
    float distToGround = m_floorY - currentY;

    if (distToGround > 5.0f && distToGround < (m_standingHeight * 1.35f)) {
        float dy = targetY - currentY; // In pixels (negative when below targetY)
        float dyMeters = dy / PhysicsUnits::PPM;
        float vy = b2Body_GetLinearVelocity(hips).y;

        // Balanced gravity counter for core pelvis/torso weight (~12 N)
        const float gravityFeedforward = -14.0f;
        float kSuspension = 150.0f;
        float kDamping = 16.0f;
        float forceY = dyMeters * kSuspension - vy * kDamping + gravityFeedforward;

        // Negative is UP, positive is DOWN:
        // Capped upward force (-85 N) prevents floating/launching while maintaining posture
        forceY = std::clamp(forceY, -85.0f, 40.0f);

        b2Body_ApplyForceToCenter(hips, b2Vec2{ 0.0f, forceY * 0.65f }, true);
        if (b2Body_IsValid(torso)) {
            b2Body_ApplyForceToCenter(torso, b2Vec2{ 0.0f, forceY * 0.35f }, true);
        }
    }

    // 2. Horizontal Locomotion Drive & Snappy Braking
    float targetVx = 0.0f;
    if (m_actionState == FighterActionState::MovingForward) {
        targetVx = m_facingDir * 4.6f;
    } else if (m_actionState == FighterActionState::MovingBackward) {
        targetVx = -m_facingDir * 3.4f;
    } else if (m_actionState == FighterActionState::Dashing) {
        targetVx = m_dashDir * 8.5f;
    }

    float currentVx = b2Body_GetLinearVelocity(hips).x;
    bool isMoving = (std::abs(targetVx) > 0.1f);
    // When moving: drive towards targetVx. When neutral: actively brake currentVx to 0 to eliminate ice-skating!
    float kDrive = isMoving ? 75.0f : 120.0f;
    float forceX = (targetVx - currentVx) * kDrive;
    forceX = std::clamp(forceX, -240.0f, 240.0f);

    b2Body_ApplyForceToCenter(hips, b2Vec2{ forceX * 0.55f, 0.0f }, true);
    if (b2Body_IsValid(torso)) {
        b2Body_ApplyForceToCenter(torso, b2Vec2{ forceX * 0.45f, 0.0f }, true);
    }

    // Active damping when standing idle/guarding to completely prevent creeping/drifting
    if (!isMoving && (m_actionState == FighterActionState::Neutral ||
                      m_actionState == FighterActionState::Crouching ||
                      m_actionState == FighterActionState::HighGuarding ||
                      m_actionState == FighterActionState::LowGuarding)) {
        auto dampVx = [](b2BodyId body, float dampFactor, float snapThreshold) {
            if (b2Body_IsValid(body)) {
                b2Vec2 v = b2Body_GetLinearVelocity(body);
                if (std::abs(v.x) < snapThreshold) {
                    v.x = 0.0f;
                } else {
                    v.x *= dampFactor;
                }
                b2Body_SetLinearVelocity(body, v);
            }
        };

        // Aggressively damp and snap horizontal velocity of hips, torso, and legs
        dampVx(hips, 0.65f, 0.35f);
        if (b2Body_IsValid(torso)) {
            dampVx(torso, 0.65f, 0.35f);
        }
        b2BodyId leftShin = m_skeleton->getBody(LimbType::LeftShin);
        b2BodyId rightShin = m_skeleton->getBody(LimbType::RightShin);
        b2BodyId leftThigh = m_skeleton->getBody(LimbType::LeftThigh);
        b2BodyId rightThigh = m_skeleton->getBody(LimbType::RightThigh);
        dampVx(leftShin, 0.60f, 0.35f);
        dampVx(rightShin, 0.60f, 0.35f);
        dampVx(leftThigh, 0.60f, 0.35f);
        dampVx(rightThigh, 0.60f, 0.35f);
    }
}

void ActiveRagdollController::applyUprightStabilizer() {
    b2BodyId torso = m_skeleton->getTorso();
    b2BodyId hips = m_skeleton->getHips();
    if (!b2Body_IsValid(torso)) return;

    if (m_actionState == FighterActionState::LaunchedJuggle ||
        m_actionState == FighterActionState::KnockedDown) {
        return;
    }

    float desiredAngle = 0.0f;

    // 1. Torso Upright PD Stabilizer
    b2Rot rotTorso = b2Body_GetRotation(torso);
    float angleTorso = b2Rot_GetAngle(rotTorso);
    float wTorso = b2Body_GetAngularVelocity(torso);
    float torqueTorso = -140.0f * (angleTorso - desiredAngle) - 12.0f * wTorso;
    torqueTorso = std::clamp(torqueTorso, -75.0f, 75.0f);
    b2Body_ApplyTorque(torso, torqueTorso, true);

    // 2. Pelvis / Hips Upright PD Stabilizer (Crucial to prevent hips pitching forward)
    if (b2Body_IsValid(hips)) {
        b2Rot rotHips = b2Body_GetRotation(hips);
        float angleHips = b2Rot_GetAngle(rotHips);
        float wHips = b2Body_GetAngularVelocity(hips);
        float torqueHips = -120.0f * (angleHips - desiredAngle) - 10.0f * wHips;
        torqueHips = std::clamp(torqueHips, -60.0f, 60.0f);
        b2Body_ApplyTorque(hips, torqueHips, true);
    }
}

} // namespace RagdollEngine
