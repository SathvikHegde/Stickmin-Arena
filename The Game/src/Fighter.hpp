#pragma once
#include <RagdollEngine/Physics/RagdollSkeleton.hpp>
#include <RagdollEngine/Physics/ActiveRagdollController.hpp>
#include <RagdollEngine/Render/RagdollRenderer.hpp>
#include <RagdollEngine/Render/CharacterRegistry.hpp>
#include <RagdollEngine/Render/JuiceFX.hpp>
#include <memory>
#include <string>
#include <algorithm>

namespace StickminGame {

class Fighter {
public:
    struct CombatAlert {
        std::string text;
        sf::Color color{ sf::Color::White };
        float timer{ 0.0f };
        float maxTimer{ 1.1f };
    };

    Fighter(b2WorldId worldId, int id, const RagdollEngine::CharacterDefinition& charDef, const sf::Vector2f& spawnPos);
    Fighter(b2WorldId worldId, int id, const std::string& fighterName, const sf::Vector2f& spawnPos, const sf::Color& accentColor);
    ~Fighter() = default;

    void respawn(const sf::Vector2f& spawnPos);
    void resetHealth();
    void update(float dt, RagdollEngine::JuiceFX& juiceFX);
    void takeDamage(float dmg);
    void addComboHit(float dmg);
    bool isDead() const { return m_health <= 0.0f; }

    // Accessors
    RagdollEngine::RagdollSkeleton* getSkeleton() { return m_skeleton.get(); }
    const RagdollEngine::RagdollSkeleton* getSkeleton() const { return m_skeleton.get(); }
    RagdollEngine::ActiveRagdollController* getController() { return m_controller.get(); }
    const RagdollEngine::ActiveRagdollController* getController() const { return m_controller.get(); }
    const RagdollEngine::CharacterDefinition& getTheme() const { return m_charDef; }
    const RagdollEngine::CharacterDefinition& getCharacterDef() const { return m_charDef; }
    void setCharacterDef(const RagdollEngine::CharacterDefinition& def);

    const std::string& getName() const { return m_name; }
    float getHealth() const { return m_health; }
    void setHealth(float h) { m_health = h; }
    float getGhostHealth() const { return m_ghostHealth; }
    void setGhostHealth(float gh) { m_ghostHealth = gh; }
    float getMaxHealth() const { return m_maxHealth; }
    int getRoundsWon() const { return m_roundsWon; }
    void setRoundsWon(int r) { m_roundsWon = r; }
    void addRoundWin() { m_roundsWon++; }
    void resetRoundsWon() { m_roundsWon = 0; }

    int getComboHits() const { return m_comboHits; }
    void setComboHits(int c) { m_comboHits = c; }
    float getComboDamage() const { return m_comboDamage; }
    void setComboDamage(float d) { m_comboDamage = d; }

    bool isInRage() const { return m_health <= 28.0f && m_health > 0.0f; }
    bool hasUsedRageArt() const { return m_rageArtUsed; }
    void setRageArtUsed(bool used) { m_rageArtUsed = used; }

    int getId() const { return m_fighterId; }
    void triggerAlert(const std::string& text, const sf::Color& color, float duration = 1.1f) {
        m_alert.text = text;
        m_alert.color = color;
        m_alert.timer = duration;
        m_alert.maxTimer = duration;
    }
    const CombatAlert& getAlert() const { return m_alert; }

private:
    b2WorldId m_worldId;
    int m_fighterId{ 1 };
    std::string m_name{ "FIGHTER" };
    sf::Color m_accentColor{ sf::Color::Red };
    bool m_rageArtUsed{ false };
    CombatAlert m_alert;

    std::unique_ptr<RagdollEngine::RagdollSkeleton> m_skeleton;
    std::unique_ptr<RagdollEngine::ActiveRagdollController> m_controller;
    RagdollEngine::CharacterDefinition m_charDef;

    float m_maxHealth{ 100.0f };
    float m_health{ 100.0f };
    float m_ghostHealth{ 100.0f };
    float m_ghostTimer{ 0.0f };

    int m_roundsWon{ 0 };
    int m_comboHits{ 0 };
    float m_comboDamage{ 0.0f };
    float m_comboTimer{ 0.0f };

    int m_trailFistL{ 0 };
    int m_trailFistR{ 1 };
    int m_trailFootL{ 2 };
    int m_trailFootR{ 3 };
};

} // namespace StickminGame
