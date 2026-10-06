#pragma once
#include "RagdollEngine/Physics/PhysicsWorld.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <vector>
#include <string>

namespace RagdollEngine {

enum class StageType {
    ToppatAirship,      // High altitude airship deck with sunset clouds, thrusters & Toppat emblems
    TheWall,            // Siberian prison yard with blizzard snowfall, watchtowers & sweeping searchlights
    BankVault           // Desert canyon bank heist with blown-open vault, gold bullion & tumbleweeds
};

struct WallImpactCrack {
    sf::Vector2f position;
    float radius{ 45.0f };
    float rotationDeg{ 0.0f };
    float lifeTime{ 15.0f };
    float maxLifeTime{ 15.0f };
    std::vector<std::vector<sf::Vector2f>> branches; // Crack fracture lines
};

struct AmbientParticle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float size{ 3.0f };
    float life{ 1.0f };
    float maxLife{ 1.0f };
    sf::Color color;
    float param{ 0.0f }; // Wobble frequency / rotation
};

class StageRenderer {
public:
    StageRenderer();

    void update(float dt);

    // 1. Draw Parallax Background (Sky, distant landscapes, mid-ground landmarks)
    void drawBackground(sf::RenderWindow& window, const sf::View& cameraView);

    // 2. Draw Themed Platforms & Arena Walls (Replaces flat grey rectangles with rich textured surfaces)
    void drawPlatforms(sf::RenderWindow& window, const std::vector<StaticPlatform>& platforms);

    // 3. Draw Stage Wall Damage Decals (Spawned on Wall Splats)
    void drawWallCracks(sf::RenderWindow& window);

    // 4. Draw Atmospheric Overlays & Dynamic Lighting (Snowstorm, searchlights, heat shimmer, foreground railings)
    void drawAtmosphereAndLighting(sf::RenderWindow& window, const sf::View& cameraView);

    // Wall splat fracture decal trigger
    void addWallCrack(const sf::Vector2f& pos, float scale = 1.0f);

    // Stage control
    void setStage(StageType stage);
    void cycleStage();
    StageType getCurrentStage() const { return m_currentStage; }
    std::string getStageName() const;
    std::string getStageSubtitle() const;
    sf::Color getStageThemeColor() const;

    // Dust / debris helper
    void spawnGroundDust(const sf::Vector2f& pos, const sf::Vector2f& vel, int count = 5);

private:
    StageType m_currentStage{ StageType::ToppatAirship };
    float m_stageTime{ 0.0f };

    // Persistent environmental particles
    std::vector<AmbientParticle> m_snowflakes;
    std::vector<AmbientParticle> m_airshipWindStreaks;
    std::vector<AmbientParticle> m_desertDust;
    std::vector<AmbientParticle> m_groundDust;

    // Tumbleweed simulation for Desert stage
    sf::Vector2f m_tumbleweedPos{ -100.0f, 790.0f };
    float m_tumbleweedRot{ 0.0f };
    float m_tumbleweedTimer{ 0.0f };

    // Wall splat cracks
    std::vector<WallImpactCrack> m_wallCracks;

    // Searchlight angles for The Wall
    float m_searchlightAngle1{ 0.0f };
    float m_searchlightAngle2{ 0.0f };

    // Helper drawing routines for specific stages
    void drawAirshipBackground(sf::RenderWindow& window, const sf::Vector2f& camOffset, const sf::Vector2f& viewCenter, const sf::Vector2f& viewSize);
    void drawTheWallBackground(sf::RenderWindow& window, const sf::Vector2f& camOffset, const sf::Vector2f& viewCenter, const sf::Vector2f& viewSize);
    void drawBankVaultBackground(sf::RenderWindow& window, const sf::Vector2f& camOffset, const sf::Vector2f& viewCenter, const sf::Vector2f& viewSize);

    void drawAirshipPlatforms(sf::RenderWindow& window, const std::vector<StaticPlatform>& platforms);
    void drawTheWallPlatforms(sf::RenderWindow& window, const std::vector<StaticPlatform>& platforms);
    void drawBankVaultPlatforms(sf::RenderWindow& window, const std::vector<StaticPlatform>& platforms);

    void initParticles();
};

} // namespace RagdollEngine
