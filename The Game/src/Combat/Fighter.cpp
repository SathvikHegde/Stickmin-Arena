#include "Fighter.hpp"

namespace StickminGame {

Fighter::Fighter(b2WorldId worldId, int id, const RagdollEngine::CharacterDefinition& charDef, const sf::Vector2f& spawnPos)
    : m_worldId(worldId), m_fighterId(id), m_charDef(charDef) {
    m_name = m_charDef.displayName;
    m_accentColor = m_charDef.accentColor;

    m_trailFistL = (id - 1) * 4 + 0;
    m_trailFistR = (id - 1) * 4 + 1;
    m_trailFootL = (id - 1) * 4 + 2;
    m_trailFootR = (id - 1) * 4 + 3;

    respawn(spawnPos);
}

Fighter::Fighter(b2WorldId worldId, int id, const std::string& fighterName, const sf::Vector2f& spawnPos, const sf::Color& accentColor)
    : Fighter(worldId, id, (id == 2 ? RagdollEngine::CharacterRegistry::getEllie() : RagdollEngine::CharacterRegistry::getHenry()), spawnPos) {
    if (!fighterName.empty()) m_charDef.displayName = fighterName;
    m_charDef.accentColor = accentColor;
    m_name = m_charDef.displayName;
    m_accentColor = accentColor;
}

void Fighter::respawn(const sf::Vector2f& spawnPos) {
    m_skeleton = std::make_unique<RagdollEngine::RagdollSkeleton>(m_worldId, m_fighterId, spawnPos);
    m_controller = std::make_unique<RagdollEngine::ActiveRagdollController>(m_skeleton.get());
    m_controller->setFacingDirection(m_fighterId == 1 ? 1 : -1);
    m_controller->setFloorY(800.0f);

    m_health = m_maxHealth;
    m_ghostHealth = m_maxHealth;
    m_ghostTimer = 0.0f;
    m_comboHits = 0;
    m_comboDamage = 0.0f;
    m_comboTimer = 0.0f;
    m_rageArtUsed = false;
    m_alert.timer = 0.0f;
    m_alert.text.clear();
}

void Fighter::resetHealth() {
    m_health = m_maxHealth;
    m_ghostHealth = m_maxHealth;
    m_ghostTimer = 0.0f;
    m_comboHits = 0;
    m_comboDamage = 0.0f;
    m_comboTimer = 0.0f;
    m_rageArtUsed = false;
    m_alert.timer = 0.0f;
    m_alert.text.clear();
    m_controller->setLimp(false);
}

void Fighter::update(float dt, RagdollEngine::JuiceFX& juiceFX) {
    // 1. Combo Timer Decay
    if (m_comboTimer > 0.0f) {
        m_comboTimer -= dt;
        if (m_comboTimer <= 0.0f) {
            m_comboHits = 0;
            m_comboDamage = 0.0f;
        }
    }

    // Combat Alert Decay
    if (m_alert.timer > 0.0f) {
        m_alert.timer -= dt;
        if (m_alert.timer < 0.0f) {
            m_alert.timer = 0.0f;
        }
    }

    // 2. Yellow Ghost Lag Bar (Tekken damage lag bar)
    if (m_ghostTimer > 0.0f) {
        m_ghostTimer -= dt;
    } else if (m_ghostHealth > m_health) {
        m_ghostHealth = std::max(m_health, m_ghostHealth - 55.0f * dt);
    }

    // 3. Electric & Motion Trail Effects
    if (m_controller->isAttackActive()) {
        const auto& move = m_controller->getCurrentMove();
        sf::Vector2f tip = m_controller->getActiveStrikingTipPixels();

        int slot = m_trailFistR;
        if (move.strikingLimb == RagdollEngine::LimbType::LeftForearm) slot = m_trailFistL;
        else if (move.strikingLimb == RagdollEngine::LimbType::RightForearm) slot = m_trailFistR;
        else if (move.strikingLimb == RagdollEngine::LimbType::LeftShin) slot = m_trailFootL;
        else if (move.strikingLimb == RagdollEngine::LimbType::RightShin) slot = m_trailFootR;

        sf::Color trailCol = move.isElectric ? sf::Color(120, 220, 255) : m_accentColor;
        juiceFX.addTrailPoint(slot, tip, trailCol, 12.0f);

        if (move.isElectric) {
            juiceFX.spawnElectricBurst(tip, sf::Color(120, 220, 255), 3, 24.0f);
            m_charDef.glowingEyes = true;
            m_charDef.eyeGlowColor = sf::Color(120, 220, 255);
        }
    } else {
        m_charDef.glowingEyes = false;
    }
}

void Fighter::takeDamage(float dmg) {
    m_health = std::max(0.0f, m_health - dmg * (1.0f / m_charDef.defenseMult));
    m_ghostTimer = 0.45f; // Hang for 0.45s before draining ghost bar
    if (m_health <= 0.0f) {
        m_controller->setLimp(true);
    }
}

void Fighter::addComboHit(float dmg) {
    m_comboHits++;
    m_comboDamage += dmg;
    m_comboTimer = 1.35f; // Combo window
}

void Fighter::setCharacterDef(const RagdollEngine::CharacterDefinition& def) {
    m_charDef = def;
    m_name = m_charDef.displayName;
    m_accentColor = m_charDef.accentColor;
}

} // namespace StickminGame
