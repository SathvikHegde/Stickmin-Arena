#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <vector>
#include <deque>
#include <string>

namespace RagdollEngine {

struct Shockwave {
    sf::Vector2f position;
    float currentRadius{ 5.0f };
    float maxRadius{ 60.0f };
    float lifeTime{ 0.25f };
    float maxLifeTime{ 0.25f };
    sf::Color color{ sf::Color(255, 230, 100) };
};

struct HitSpark {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float lifeTime{ 0.2f };
    float maxLifeTime{ 0.2f };
    float length{ 15.0f };
    sf::Color color{ sf::Color(255, 240, 150) };
};

struct ImpactStarburst {
    sf::Vector2f position;
    float currentScale{ 0.1f };
    float maxScale{ 1.0f };
    float rotationDeg{ 0.0f };
    float lifeTime{ 0.16f };
    float maxLifeTime{ 0.16f };
    sf::Color color{ sf::Color(255, 245, 180) };
};

struct ElectricSegment {
    sf::Vector2f p1;
    sf::Vector2f p2;
    float lifeTime{ 0.14f };
    float maxLifeTime{ 0.14f };
    sf::Color color{ sf::Color(120, 220, 255) };
};

struct BlockBarrier {
    sf::Vector2f position;
    float radius{ 24.0f };
    float lifeTime{ 0.20f };
    float maxLifeTime{ 0.20f };
    sf::Color color{ sf::Color(100, 200, 255) };
};

struct TrailSegment {
    sf::Vector2f pos;
    float width{ 8.0f };
    float lifeTime{ 0.15f };
    float maxLifeTime{ 0.15f };
    sf::Color color{ sf::Color::White };
};

struct FloatingTextItem {
    sf::Vector2f position;
    sf::Vector2f velocity;
    std::string text;
    float lifeTime{ 0.65f };
    float maxLifeTime{ 0.65f };
    sf::Color color{ sf::Color::Yellow };
    float scale{ 1.0f };
};

class JuiceFX {
public:
    JuiceFX();

    void setFont(const sf::Font* font) { m_font = font; }

    void update(float dt);
    void draw(sf::RenderWindow& window);

    // Effect triggers
    void spawnImpact(const sf::Vector2f& pos, const sf::Vector2f& impactDir, const sf::Color& color = sf::Color(255, 235, 120), bool heavy = false);
    void spawnBlockEffect(const sf::Vector2f& pos);
    void spawnElectricBurst(const sf::Vector2f& center, const sf::Color& color = sf::Color(120, 220, 255), int boltCount = 6, float radius = 35.0f);
    void spawnFloatingText(const sf::Vector2f& pos, const std::string& text, const sf::Color& color, float scale = 1.0f);
    void addTrailPoint(int trailSlot, const sf::Vector2f& point, const sf::Color& color, float width = 8.0f);

    void triggerScreenFlash(float duration = 0.08f, const sf::Color& color = sf::Color(255, 255, 255, 100));

private:
    std::vector<Shockwave> m_shockwaves;
    std::vector<HitSpark> m_sparks;
    std::vector<ImpactStarburst> m_starbursts;
    std::vector<ElectricSegment> m_electricArcs;
    std::vector<BlockBarrier> m_blocks;
    std::vector<FloatingTextItem> m_floatingTexts;

    // Up to 8 persistent limb trails (P1 fists/feet, P2 fists/feet)
    std::vector<TrailSegment> m_trails[8];

    const sf::Font* m_font{ nullptr };

    float m_screenFlashTimer{ 0.0f };
    float m_screenFlashDuration{ 0.0f };
    sf::Color m_screenFlashColor{ sf::Color::Transparent };
};

} // namespace RagdollEngine
