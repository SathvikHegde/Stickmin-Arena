#include "RagdollEngine/Render/StageRenderer.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <cmath>
#include <random>
#include <algorithm>

namespace RagdollEngine {

StageRenderer::StageRenderer() {
    initParticles();
}

void StageRenderer::initParticles() {
    static std::mt19937 rng(42);
    std::uniform_real_distribution<float> xDist(-200.0f, 1800.0f);
    std::uniform_real_distribution<float> yDist(-100.0f, 950.0f);
    std::uniform_real_distribution<float> speedDist(40.0f, 160.0f);
    std::uniform_real_distribution<float> sizeDist(2.0f, 5.5f);
    std::uniform_real_distribution<float> phaseDist(0.0f, 6.28f);

    // 1. Snowflakes for The Wall
    m_snowflakes.clear();
    for (int i = 0; i < 110; ++i) {
        AmbientParticle p;
        p.pos = sf::Vector2f(xDist(rng), yDist(rng));
        p.vel = sf::Vector2f(speedDist(rng) * 0.45f, speedDist(rng));
        p.size = sizeDist(rng);
        p.param = phaseDist(rng); // Sine wave drift phase
        p.color = sf::Color(240, 245, 255, static_cast<std::uint8_t>(130 + p.size * 22.0f));
        m_snowflakes.push_back(p);
    }

    // 2. Wind Streaks for Airship Deck
    m_airshipWindStreaks.clear();
    std::uniform_real_distribution<float> streakSpeed(350.0f, 750.0f);
    for (int i = 0; i < 28; ++i) {
        AmbientParticle p;
        p.pos = sf::Vector2f(xDist(rng), yDist(rng));
        p.vel = sf::Vector2f(streakSpeed(rng), 0.0f);
        p.size = 20.0f + streakSpeed(rng) * 0.05f; // Length
        p.color = sf::Color(255, 255, 255, 45);
        m_airshipWindStreaks.push_back(p);
    }

    // 3. Desert Dust for Bank Vault
    m_desertDust.clear();
    std::uniform_real_distribution<float> dustSpeed(25.0f, 90.0f);
    for (int i = 0; i < 45; ++i) {
        AmbientParticle p;
        p.pos = sf::Vector2f(xDist(rng), yDist(rng));
        p.vel = sf::Vector2f(dustSpeed(rng) * 1.5f, -dustSpeed(rng) * 0.2f);
        p.size = 2.0f + sizeDist(rng) * 0.8f;
        p.color = sf::Color(225, 175, 95, static_cast<std::uint8_t>(60 + sizeDist(rng) * 15.0f));
        p.param = phaseDist(rng);
        m_desertDust.push_back(p);
    }
}

void StageRenderer::setStage(StageType stage) {
    m_currentStage = stage;
}

void StageRenderer::cycleStage() {
    int next = (static_cast<int>(m_currentStage) + 1) % 3;
    m_currentStage = static_cast<StageType>(next);
}

std::string StageRenderer::getStageName() const {
    switch (m_currentStage) {
        case StageType::ToppatAirship: return "TOPPAT AIRSHIP DECK";
        case StageType::TheWall:       return "THE WALL - PRISON YARD";
        case StageType::BankVault:      return "THE BANK VAULT";
    }
    return "ARENA";
}

std::string StageRenderer::getStageSubtitle() const {
    switch (m_currentStage) {
        case StageType::ToppatAirship: return "ALTITUDE: 14,000 FT // SUNSET FLEET";
        case StageType::TheWall:       return "COMPLEX SECTOR 4 // SIBERIAN BLIZZARD";
        case StageType::BankVault:      return "DESERT CANYON HEIST // BREACHED SAFES";
    }
    return "";
}

sf::Color StageRenderer::getStageThemeColor() const {
    switch (m_currentStage) {
        case StageType::ToppatAirship: return sf::Color(245, 55, 65);
        case StageType::TheWall:       return sf::Color(80, 200, 255);
        case StageType::BankVault:      return sf::Color(255, 195, 45);
    }
    return sf::Color::White;
}

void StageRenderer::addWallCrack(const sf::Vector2f& pos, float scale) {
    WallImpactCrack crack;
    crack.position = pos;
    crack.radius = 35.0f * scale;
    crack.lifeTime = 15.0f;
    crack.maxLifeTime = 15.0f;

    static std::mt19937 rng(1234);
    std::uniform_real_distribution<float> rotDist(0.0f, 6.28f);
    crack.rotationDeg = rotDist(rng);

    // Procedural jagged fracture branches radiating outward
    int numBranches = 5 + (rng() % 3);
    for (int b = 0; b < numBranches; ++b) {
        std::vector<sf::Vector2f> branch;
        float baseAngle = (6.28318f / numBranches) * b + rotDist(rng) * 0.3f;
        sf::Vector2f cur = pos;
        branch.push_back(cur);

        int segments = 3 + (rng() % 3);
        float segLen = (crack.radius / segments);
        for (int s = 0; s < segments; ++s) {
            float angleOffset = ((rng() % 100) / 100.0f - 0.5f) * 0.8f;
            float ang = baseAngle + angleOffset;
            cur += sf::Vector2f(std::cos(ang) * segLen, std::sin(ang) * segLen);
            branch.push_back(cur);
        }
        crack.branches.push_back(branch);
    }
    m_wallCracks.push_back(crack);
}

void StageRenderer::spawnGroundDust(const sf::Vector2f& pos, const sf::Vector2f& vel, int count) {
    static std::mt19937 rng(555);
    std::uniform_real_distribution<float> angSpread(-0.8f, 0.8f);
    std::uniform_real_distribution<float> spdSpread(0.6f, 1.4f);

    sf::Color dustColor = (m_currentStage == StageType::TheWall) ? sf::Color(240, 248, 255, 180)
                        : (m_currentStage == StageType::BankVault) ? sf::Color(215, 175, 110, 180)
                        : sf::Color(180, 190, 205, 160);

    for (int i = 0; i < count; ++i) {
        AmbientParticle p;
        p.pos = pos;
        float ang = std::atan2(vel.y, vel.x) + angSpread(rng);
        float spd = std::sqrt(vel.x * vel.x + vel.y * vel.y) * spdSpread(rng);
        p.vel = sf::Vector2f(std::cos(ang) * spd, std::sin(ang) * spd - 25.0f);
        p.size = 5.0f + (rng() % 5);
        p.life = 0.35f;
        p.maxLife = 0.35f;
        p.color = dustColor;
        m_groundDust.push_back(p);
    }
}

void StageRenderer::update(float dt) {
    m_stageTime += dt;

    // 1. Update Snowflakes
    for (auto& p : m_snowflakes) {
        p.pos.y += p.vel.y * dt;
        p.param += dt * 2.8f;
        p.pos.x += (p.vel.x + std::sin(p.param) * 45.0f) * dt;

        if (p.pos.y > 930.0f) {
            p.pos.y = -20.0f;
            p.pos.x = -100.0f + static_cast<float>(rand() % 1800);
        }
        if (p.pos.x > 1750.0f) p.pos.x = -50.0f;
    }

    // 2. Update Airship Wind Streaks
    for (auto& p : m_airshipWindStreaks) {
        p.pos.x -= p.vel.x * dt;
        if (p.pos.x < -150.0f) {
            p.pos.x = 1750.0f;
            p.pos.y = 50.0f + static_cast<float>(rand() % 800);
        }
    }

    // 3. Update Desert Dust
    for (auto& p : m_desertDust) {
        p.pos.x += p.vel.x * dt;
        p.param += dt * 1.5f;
        p.pos.y += (p.vel.y + std::sin(p.param) * 15.0f) * dt;

        if (p.pos.x > 1750.0f) {
            p.pos.x = -50.0f;
            p.pos.y = 200.0f + static_cast<float>(rand() % 700);
        }
    }

    // 4. Update Tumbleweed in Desert Stage
    if (m_currentStage == StageType::BankVault) {
        m_tumbleweedPos.x += 165.0f * dt;
        m_tumbleweedRot += 280.0f * dt;
        // Natural floor bouncing
        m_tumbleweedPos.y = 785.0f - std::abs(std::sin(m_tumbleweedPos.x * 0.02f)) * 32.0f;

        if (m_tumbleweedPos.x > 1800.0f) {
            m_tumbleweedTimer += dt;
            if (m_tumbleweedTimer > 6.0f) {
                m_tumbleweedPos.x = -150.0f;
                m_tumbleweedTimer = 0.0f;
            }
        }
    }

    // 5. Update Ground Dust Puffs
    for (auto it = m_groundDust.begin(); it != m_groundDust.end();) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_groundDust.erase(it);
        } else {
            it->pos += it->vel * dt;
            it->vel *= (1.0f - 3.5f * dt);
            it->size += 14.0f * dt; // Expand as it dissipates
            ++it;
        }
    }

    // 6. Update Wall Splat Cracks
    for (auto it = m_wallCracks.begin(); it != m_wallCracks.end();) {
        it->lifeTime -= dt;
        if (it->lifeTime <= 0.0f) {
            it = m_wallCracks.erase(it);
        } else {
            ++it;
        }
    }

    // 7. Update Searchlight Oscillation for The Wall
    m_searchlightAngle1 = std::sin(m_stageTime * 0.45f) * 36.0f;
    m_searchlightAngle2 = std::sin(m_stageTime * 0.38f + 2.1f) * 32.0f;
}

// -----------------------------------------------------------------------------
// 1. BACKGROUND PASS (PARALLAX + SKY)
// -----------------------------------------------------------------------------
void StageRenderer::drawBackground(sf::RenderWindow& window, const sf::View& cameraView) {
    sf::Vector2f viewCenter = cameraView.getCenter();
    sf::Vector2f viewSize = cameraView.getSize();
    sf::Vector2f camOffset = viewCenter - sf::Vector2f(800.0f, 450.0f);

    switch (m_currentStage) {
        case StageType::ToppatAirship:
            drawAirshipBackground(window, camOffset, viewCenter, viewSize);
            break;
        case StageType::TheWall:
            drawTheWallBackground(window, camOffset, viewCenter, viewSize);
            break;
        case StageType::BankVault:
            drawBankVaultBackground(window, camOffset, viewCenter, viewSize);
            break;
    }
}

void StageRenderer::drawAirshipBackground(sf::RenderWindow& window, const sf::Vector2f& camOffset, const sf::Vector2f& viewCenter, const sf::Vector2f& viewSize) {
    // A. Dramatic Sunset Sky Gradient (Parallax 0x - Infinite depth)
    sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
    float l = viewCenter.x - viewSize.x * 0.75f;
    float r = viewCenter.x + viewSize.x * 0.75f;
    float t = viewCenter.y - viewSize.y * 0.75f;
    float b = viewCenter.y + viewSize.y * 0.75f;
    float mid1 = t + (b - t) * 0.35f;
    float mid2 = t + (b - t) * 0.65f;

    sf::Color cTop(20, 16, 42);       // Deep twilight violet
    sf::Color cMid1(145, 34, 62);     // Rich Toppat crimson-rose
    sf::Color cMid2(235, 95, 35);      // Fiery twilight orange
    sf::Color cBot(255, 190, 70);      // Golden horizon haze

    sky.append(sf::Vertex{ sf::Vector2f(l, t), cTop });
    sky.append(sf::Vertex{ sf::Vector2f(r, t), cTop });
    sky.append(sf::Vertex{ sf::Vector2f(l, mid1), cMid1 });
    sky.append(sf::Vertex{ sf::Vector2f(r, mid1), cMid1 });
    sky.append(sf::Vertex{ sf::Vector2f(l, mid2), cMid2 });
    sky.append(sf::Vertex{ sf::Vector2f(r, mid2), cMid2 });
    sky.append(sf::Vertex{ sf::Vector2f(l, b), cBot });
    sky.append(sf::Vertex{ sf::Vector2f(r, b), cBot });
    window.draw(sky);

    // B. Distant Sunset Cloud Layer (Parallax factor 0.12x)
    sf::Vector2f cloudOffset = -camOffset * 0.12f;
    for (int i = 0; i < 9; ++i) {
        float cx = -250.0f + i * 260.0f + cloudOffset.x;
        float cy = 520.0f + std::sin(i * 1.7f) * 45.0f + cloudOffset.y;
        sf::CircleShape cloud(130.0f);
        cloud.setScale(sf::Vector2f(1.6f, 0.55f));
        cloud.setOrigin(sf::Vector2f(130.0f, 130.0f));
        cloud.setPosition(sf::Vector2f(cx, cy));
        cloud.setFillColor(sf::Color(245, 140, 80, 110));
        window.draw(cloud);

        // Warm golden rim highlight on top of clouds
        sf::CircleShape rim(124.0f);
        rim.setScale(sf::Vector2f(1.55f, 0.48f));
        rim.setOrigin(sf::Vector2f(124.0f, 124.0f));
        rim.setPosition(sf::Vector2f(cx, cy - 8.0f));
        rim.setFillColor(sf::Color(255, 215, 130, 75));
        window.draw(rim);
    }

    // C. Giant Toppat Airship Hull & Tail Fin (Parallax factor 0.32x)
    sf::Vector2f airshipOffset = -camOffset * 0.32f;
    float shipCenterX = 800.0f + airshipOffset.x;
    float shipCenterY = 380.0f + airshipOffset.y;

    // 1. Massive Red Airship Main Body Silhouette
    sf::RectangleShape airshipBody(sf::Vector2f(1300.0f, 240.0f));
    airshipBody.setOrigin(sf::Vector2f(650.0f, 120.0f));
    airshipBody.setPosition(sf::Vector2f(shipCenterX, shipCenterY));
    airshipBody.setFillColor(sf::Color(165, 24, 38));
    airshipBody.setOutlineColor(sf::Color(95, 12, 22));
    airshipBody.setOutlineThickness(3.5f);
    window.draw(airshipBody);

    // Darker lower hull shading
    sf::RectangleShape hullShade(sf::Vector2f(1300.0f, 75.0f));
    hullShade.setOrigin(sf::Vector2f(650.0f, 0.0f));
    hullShade.setPosition(sf::Vector2f(shipCenterX, shipCenterY + 45.0f));
    hullShade.setFillColor(sf::Color(115, 14, 25));
    window.draw(hullShade);

    // 2. Giant Toppat Tail Fin Stabilizer
    sf::ConvexShape tailFin;
    tailFin.setPointCount(4);
    tailFin.setPoint(0, sf::Vector2f(shipCenterX - 550.0f, shipCenterY - 120.0f));
    tailFin.setPoint(1, sf::Vector2f(shipCenterX - 660.0f, shipCenterY - 260.0f));
    tailFin.setPoint(2, sf::Vector2f(shipCenterX - 480.0f, shipCenterY - 260.0f));
    tailFin.setPoint(3, sf::Vector2f(shipCenterX - 420.0f, shipCenterY - 120.0f));
    tailFin.setFillColor(sf::Color(165, 24, 38));
    tailFin.setOutlineColor(sf::Color(95, 12, 22));
    tailFin.setOutlineThickness(3.0f);
    window.draw(tailFin);

    // Toppat Gold Band on the Tail Fin
    sf::RectangleShape goldBand(sf::Vector2f(140.0f, 22.0f));
    goldBand.setOrigin(sf::Vector2f(70.0f, 11.0f));
    goldBand.setPosition(sf::Vector2f(shipCenterX - 570.0f, shipCenterY - 220.0f));
    goldBand.setFillColor(sf::Color(255, 215, 0));
    goldBand.setOutlineColor(sf::Color(180, 140, 0));
    goldBand.setOutlineThickness(1.5f);
    window.draw(goldBand);

    // 3. Glowing Bridge Windows (Toppat Bridge Deck)
    for (int w = 0; w < 7; ++w) {
        sf::RectangleShape win(sf::Vector2f(28.0f, 18.0f));
        win.setOrigin(sf::Vector2f(14.0f, 9.0f));
        win.setPosition(sf::Vector2f(shipCenterX + 180.0f + w * 45.0f, shipCenterY - 45.0f));
        win.setFillColor(sf::Color(255, 240, 130, 230));
        win.setOutlineColor(sf::Color(40, 20, 10));
        win.setOutlineThickness(1.5f);
        window.draw(win);
    }

    // 4. Rotating Radar Antenna Mast
    float radarAng = m_stageTime * 150.0f;
    float radarMastX = shipCenterX + 280.0f;
    float radarMastY = shipCenterY - 120.0f;
    sf::RectangleShape mast(sf::Vector2f(6.0f, 40.0f));
    mast.setOrigin(sf::Vector2f(3.0f, 40.0f));
    mast.setPosition(sf::Vector2f(radarMastX, radarMastY));
    mast.setFillColor(sf::Color(45, 50, 60));
    window.draw(mast);

    sf::RectangleShape dish(sf::Vector2f(std::abs(std::cos(radarAng * 0.01745f)) * 34.0f, 6.0f));
    dish.setOrigin(sf::Vector2f(dish.getSize().x * 0.5f, 3.0f));
    dish.setPosition(sf::Vector2f(radarMastX, radarMastY - 40.0f));
    dish.setFillColor(sf::Color(210, 220, 235));
    window.draw(dish);

    // 5. Pulsing Red Navigation Beacon Light
    float beaconPulse = (std::sin(m_stageTime * 5.0f) + 1.0f) * 0.5f;
    sf::CircleShape beaconGlow(18.0f + beaconPulse * 10.0f);
    beaconGlow.setOrigin(sf::Vector2f(beaconGlow.getRadius(), beaconGlow.getRadius()));
    beaconGlow.setPosition(sf::Vector2f(shipCenterX - 660.0f, shipCenterY - 260.0f));
    beaconGlow.setFillColor(sf::Color(255, 30, 30, static_cast<std::uint8_t>(80 + beaconPulse * 150.0f)));
    window.draw(beaconGlow);

    sf::CircleShape beaconCore(4.0f);
    beaconCore.setOrigin(sf::Vector2f(4.0f, 4.0f));
    beaconCore.setPosition(beaconGlow.getPosition());
    beaconCore.setFillColor(sf::Color(255, 230, 230));
    window.draw(beaconCore);

    // 6. Giant Jet Turbine Thrusters on Underside
    sf::RectangleShape turbine(sf::Vector2f(160.0f, 52.0f));
    turbine.setOrigin(sf::Vector2f(80.0f, 26.0f));
    turbine.setPosition(sf::Vector2f(shipCenterX - 360.0f, shipCenterY + 115.0f));
    turbine.setFillColor(sf::Color(55, 60, 72));
    turbine.setOutlineColor(sf::Color(25, 28, 35));
    turbine.setOutlineThickness(2.5f);
    window.draw(turbine);

    // Glowing cyan turbine exhaust
    float flameLen = 45.0f + std::sin(m_stageTime * 18.0f) * 12.0f;
    sf::ConvexShape flame;
    flame.setPointCount(3);
    flame.setPoint(0, sf::Vector2f(turbine.getPosition().x - 80.0f, turbine.getPosition().y - 18.0f));
    flame.setPoint(1, sf::Vector2f(turbine.getPosition().x - 80.0f - flameLen, turbine.getPosition().y));
    flame.setPoint(2, sf::Vector2f(turbine.getPosition().x - 80.0f, turbine.getPosition().y + 18.0f));
    flame.setFillColor(sf::Color(80, 210, 255, 210));
    window.draw(flame);

    // D. Foreground Parallax Railings (Behind arena floor, Parallax factor 0.55x)
    sf::Vector2f railOffset = -camOffset * 0.55f;
    for (int r = 0; r < 14; ++r) {
        float rx = 100.0f + r * 110.0f + railOffset.x;
        float ry = 770.0f + railOffset.y;
        // Stanchion post
        sf::RectangleShape post(sf::Vector2f(5.0f, 48.0f));
        post.setOrigin(sf::Vector2f(2.5f, 48.0f));
        post.setPosition(sf::Vector2f(rx, ry));
        post.setFillColor(sf::Color(70, 78, 92));
        window.draw(post);
    }
    sf::RectangleShape railTop(sf::Vector2f(1600.0f, 5.0f));
    railTop.setOrigin(sf::Vector2f(800.0f, 2.5f));
    railTop.setPosition(sf::Vector2f(800.0f + railOffset.x, 724.0f + railOffset.y));
    railTop.setFillColor(sf::Color(100, 110, 128));
    window.draw(railTop);
}

void StageRenderer::drawTheWallBackground(sf::RenderWindow& window, const sf::Vector2f& camOffset, const sf::Vector2f& viewCenter, const sf::Vector2f& viewSize) {
    // A. Arctic Blizzard Night Sky
    sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
    float l = viewCenter.x - viewSize.x * 0.75f;
    float r = viewCenter.x + viewSize.x * 0.75f;
    float t = viewCenter.y - viewSize.y * 0.75f;
    float b = viewCenter.y + viewSize.y * 0.75f;

    sf::Color cTop(8, 14, 24);        // Freezing navy
    sf::Color cMid(20, 34, 50);       // Frosty slate
    sf::Color cBot(42, 65, 88);       // Cold ice fog

    sky.append(sf::Vertex{ sf::Vector2f(l, t), cTop });
    sky.append(sf::Vertex{ sf::Vector2f(r, t), cTop });
    sky.append(sf::Vertex{ sf::Vector2f(l, (t + b) * 0.5f), cMid });
    sky.append(sf::Vertex{ sf::Vector2f(r, (t + b) * 0.5f), cMid });
    sky.append(sf::Vertex{ sf::Vector2f(l, b), cBot });
    sky.append(sf::Vertex{ sf::Vector2f(r, b), cBot });
    window.draw(sky);

    // Aurora Borealis Green/Cyan Glow Ribbon
    sf::ConvexShape aurora;
    aurora.setPointCount(6);
    float wave = std::sin(m_stageTime * 0.8f) * 25.0f;
    aurora.setPoint(0, sf::Vector2f(l, t + 120.0f));
    aurora.setPoint(1, sf::Vector2f(viewCenter.x - 300.0f, t + 90.0f + wave));
    aurora.setPoint(2, sf::Vector2f(viewCenter.x + 300.0f, t + 140.0f - wave));
    aurora.setPoint(3, sf::Vector2f(r, t + 110.0f));
    aurora.setPoint(4, sf::Vector2f(viewCenter.x + 100.0f, t + 220.0f));
    aurora.setPoint(5, sf::Vector2f(viewCenter.x - 200.0f, t + 240.0f));
    aurora.setFillColor(sf::Color(45, 210, 165, 38));
    window.draw(aurora);

    // B. Distant Jagged Siberian Mountains (Parallax 0.12x)
    sf::Vector2f mtnOffset = -camOffset * 0.12f;
    sf::ConvexShape mountains;
    mountains.setPointCount(9);
    mountains.setPoint(0, sf::Vector2f(-300.0f + mtnOffset.x, 800.0f));
    mountains.setPoint(1, sf::Vector2f(-100.0f + mtnOffset.x, 380.0f + mtnOffset.y));
    mountains.setPoint(2, sf::Vector2f(250.0f + mtnOffset.x, 510.0f + mtnOffset.y));
    mountains.setPoint(3, sf::Vector2f(600.0f + mtnOffset.x, 330.0f + mtnOffset.y));
    mountains.setPoint(4, sf::Vector2f(950.0f + mtnOffset.x, 480.0f + mtnOffset.y));
    mountains.setPoint(5, sf::Vector2f(1350.0f + mtnOffset.x, 310.0f + mtnOffset.y));
    mountains.setPoint(6, sf::Vector2f(1650.0f + mtnOffset.x, 460.0f + mtnOffset.y));
    mountains.setPoint(7, sf::Vector2f(1900.0f + mtnOffset.x, 390.0f + mtnOffset.y));
    mountains.setPoint(8, sf::Vector2f(2100.0f + mtnOffset.x, 800.0f));
    mountains.setFillColor(sf::Color(22, 32, 48));
    window.draw(mountains);

    // Mountain Snow Caps
    sf::ConvexShape snowPeak;
    snowPeak.setPointCount(3);
    snowPeak.setPoint(0, sf::Vector2f(600.0f + mtnOffset.x, 330.0f + mtnOffset.y));
    snowPeak.setPoint(1, sf::Vector2f(540.0f + mtnOffset.x, 390.0f + mtnOffset.y));
    snowPeak.setPoint(2, sf::Vector2f(670.0f + mtnOffset.x, 400.0f + mtnOffset.y));
    snowPeak.setFillColor(sf::Color(180, 205, 230, 190));
    window.draw(snowPeak);

    // C. Massive Concrete Fortress Wall of "The Wall" (Parallax 0.32x)
    sf::Vector2f wallOffset = -camOffset * 0.32f;
    float wallBaseX = 800.0f + wallOffset.x;
    float wallBaseY = 480.0f + wallOffset.y;

    // Concrete fortress facade
    sf::RectangleShape fortWall(sf::Vector2f(1700.0f, 380.0f));
    fortWall.setOrigin(sf::Vector2f(850.0f, 0.0f));
    fortWall.setPosition(sf::Vector2f(wallBaseX, wallBaseY));
    fortWall.setFillColor(sf::Color(38, 44, 54));
    fortWall.setOutlineColor(sf::Color(20, 24, 30));
    fortWall.setOutlineThickness(3.0f);
    window.draw(fortWall);

    // Vertical buttress pillars along the wall
    for (int p = 0; p < 6; ++p) {
        float px = wallBaseX - 600.0f + p * 240.0f;
        sf::RectangleShape pillar(sf::Vector2f(32.0f, 380.0f));
        pillar.setOrigin(sf::Vector2f(16.0f, 0.0f));
        pillar.setPosition(sf::Vector2f(px, wallBaseY));
        pillar.setFillColor(sf::Color(30, 35, 44));
        window.draw(pillar);
    }

    // Barbed Wire fencing coils across the crest
    for (int w = 0; w < 16; ++w) {
        float wx = wallBaseX - 700.0f + w * 95.0f;
        sf::CircleShape coil(14.0f);
        coil.setOrigin(sf::Vector2f(14.0f, 14.0f));
        coil.setPosition(sf::Vector2f(wx, wallBaseY - 14.0f));
        coil.setFillColor(sf::Color::Transparent);
        coil.setOutlineColor(sf::Color(140, 155, 175, 180));
        coil.setOutlineThickness(2.0f);
        window.draw(coil);
    }

    // D. Wooden Sniper Watchtower (Siberian Guard Station)
    float towerX = wallBaseX + 480.0f;
    float towerY = wallBaseY - 140.0f;

    // Tower legs
    sf::RectangleShape leg1(sf::Vector2f(6.0f, 160.0f));
    leg1.setOrigin(sf::Vector2f(3.0f, 0.0f));
    leg1.setPosition(sf::Vector2f(towerX - 25.0f, towerY));
    leg1.setFillColor(sf::Color(26, 30, 36));
    window.draw(leg1);

    sf::RectangleShape leg2(sf::Vector2f(6.0f, 160.0f));
    leg2.setOrigin(sf::Vector2f(3.0f, 0.0f));
    leg2.setPosition(sf::Vector2f(towerX + 25.0f, towerY));
    leg2.setFillColor(sf::Color(26, 30, 36));
    window.draw(leg2);

    // Tower cabin
    sf::RectangleShape cabin(sf::Vector2f(80.0f, 65.0f));
    cabin.setOrigin(sf::Vector2f(40.0f, 65.0f));
    cabin.setPosition(sf::Vector2f(towerX, towerY));
    cabin.setFillColor(sf::Color(44, 40, 38));
    cabin.setOutlineColor(sf::Color(18, 16, 15));
    cabin.setOutlineThickness(2.5f);
    window.draw(cabin);

    // Cabin snow roof
    sf::RectangleShape roof(sf::Vector2f(96.0f, 14.0f));
    roof.setOrigin(sf::Vector2f(48.0f, 14.0f));
    roof.setPosition(sf::Vector2f(towerX, towerY - 65.0f));
    roof.setFillColor(sf::Color(225, 235, 250));
    window.draw(roof);

    // Lit Sniper Slit Window
    sf::RectangleShape slit(sf::Vector2f(55.0f, 12.0f));
    slit.setOrigin(sf::Vector2f(27.5f, 6.0f));
    slit.setPosition(sf::Vector2f(towerX, towerY - 32.0f));
    slit.setFillColor(sf::Color(255, 220, 110, 220));
    window.draw(slit);
}

void StageRenderer::drawBankVaultBackground(sf::RenderWindow& window, const sf::Vector2f& camOffset, const sf::Vector2f& viewCenter, const sf::Vector2f& viewSize) {
    // A. Blazing Desert Canyon Sky
    sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
    float l = viewCenter.x - viewSize.x * 0.75f;
    float r = viewCenter.x + viewSize.x * 0.75f;
    float t = viewCenter.y - viewSize.y * 0.75f;
    float b = viewCenter.y + viewSize.y * 0.75f;

    sf::Color cTop(25, 85, 160);       // Rich desert azure
    sf::Color cMid(80, 155, 195);      // Cyan desert sky
    sf::Color cBot(235, 175, 95);      // Warm dusty ochre horizon

    sky.append(sf::Vertex{ sf::Vector2f(l, t), cTop });
    sky.append(sf::Vertex{ sf::Vector2f(r, t), cTop });
    sky.append(sf::Vertex{ sf::Vector2f(l, (t + b) * 0.45f), cMid });
    sky.append(sf::Vertex{ sf::Vector2f(r, (t + b) * 0.45f), cMid });
    sky.append(sf::Vertex{ sf::Vector2f(l, b), cBot });
    sky.append(sf::Vertex{ sf::Vector2f(r, b), cBot });
    window.draw(sky);

    // Radiant Sun Disc
    sf::CircleShape sun(45.0f);
    sun.setOrigin(sf::Vector2f(45.0f, 45.0f));
    sun.setPosition(sf::Vector2f(viewCenter.x + 350.0f - camOffset.x * 0.05f, viewCenter.y - 180.0f));
    sun.setFillColor(sf::Color(255, 250, 210, 240));
    window.draw(sun);

    // B. Distant Red Rock Mesas & Buttes (Parallax 0.12x)
    sf::Vector2f mesaOffset = -camOffset * 0.12f;
    sf::ConvexShape mesa;
    mesa.setPointCount(8);
    mesa.setPoint(0, sf::Vector2f(-200.0f + mesaOffset.x, 800.0f));
    mesa.setPoint(1, sf::Vector2f(50.0f + mesaOffset.x, 460.0f + mesaOffset.y));
    mesa.setPoint(2, sf::Vector2f(320.0f + mesaOffset.x, 460.0f + mesaOffset.y));
    mesa.setPoint(3, sf::Vector2f(440.0f + mesaOffset.x, 800.0f));
    mesa.setPoint(4, sf::Vector2f(980.0f + mesaOffset.x, 800.0f));
    mesa.setPoint(5, sf::Vector2f(1080.0f + mesaOffset.x, 420.0f + mesaOffset.y));
    mesa.setPoint(6, sf::Vector2f(1420.0f + mesaOffset.x, 420.0f + mesaOffset.y));
    mesa.setPoint(7, sf::Vector2f(1580.0f + mesaOffset.x, 800.0f));
    mesa.setFillColor(sf::Color(175, 65, 42));
    window.draw(mesa);

    // C. The Fortified Bank Vault Cliff Structure (Parallax 0.32x)
    sf::Vector2f vaultOffset = -camOffset * 0.32f;
    float vaultX = 800.0f + vaultOffset.x;
    float vaultY = 490.0f + vaultOffset.y;

    // Heavy canyon rock cliff
    sf::RectangleShape cliff(sf::Vector2f(1500.0f, 380.0f));
    cliff.setOrigin(sf::Vector2f(750.0f, 0.0f));
    cliff.setPosition(sf::Vector2f(vaultX, vaultY));
    cliff.setFillColor(sf::Color(135, 52, 32));
    cliff.setOutlineColor(sf::Color(80, 26, 14));
    cliff.setOutlineThickness(3.5f);
    window.draw(cliff);

    // Steel vault outer frame
    sf::RectangleShape vaultFrame(sf::Vector2f(380.0f, 260.0f));
    vaultFrame.setOrigin(sf::Vector2f(190.0f, 130.0f));
    vaultFrame.setPosition(sf::Vector2f(vaultX, vaultY + 140.0f));
    vaultFrame.setFillColor(sf::Color(55, 60, 70));
    vaultFrame.setOutlineColor(sf::Color(25, 28, 32));
    vaultFrame.setOutlineThickness(4.0f);
    window.draw(vaultFrame);

    // Dark breached vault doorway interior (The Safe)
    sf::RectangleShape safeInterior(sf::Vector2f(320.0f, 220.0f));
    safeInterior.setOrigin(sf::Vector2f(160.0f, 110.0f));
    safeInterior.setPosition(vaultFrame.getPosition());
    safeInterior.setFillColor(sf::Color(18, 16, 20));
    window.draw(safeInterior);

    // Piles of Stolen Gold Bullion Bars in the Safe
    for (int g = 0; g < 8; ++g) {
        sf::RectangleShape gold(sf::Vector2f(28.0f, 12.0f));
        gold.setOrigin(sf::Vector2f(14.0f, 6.0f));
        gold.setPosition(sf::Vector2f(vaultX - 90.0f + (g % 4) * 32.0f, vaultY + 210.0f - (g / 4) * 14.0f));
        gold.setFillColor(sf::Color(255, 215, 0));
        gold.setOutlineColor(sf::Color(160, 130, 0));
        gold.setOutlineThickness(1.2f);
        window.draw(gold);
    }

    // THE TUNISIAN DIAMOND (The iconic Henry Stickmin diamond on a display stand!)
    sf::Vector2f diaPos(vaultX + 45.0f, vaultY + 175.0f);
    // Pedestal
    sf::RectangleShape pedestal(sf::Vector2f(32.0f, 42.0f));
    pedestal.setOrigin(sf::Vector2f(16.0f, 0.0f));
    pedestal.setPosition(sf::Vector2f(diaPos.x, diaPos.y + 16.0f));
    pedestal.setFillColor(sf::Color(145, 20, 30)); // Crimson velvet
    pedestal.setOutlineColor(sf::Color(255, 215, 0));
    pedestal.setOutlineThickness(1.5f);
    window.draw(pedestal);

    // Diamond Gem Shape
    sf::ConvexShape diamond;
    diamond.setPointCount(5);
    diamond.setPoint(0, sf::Vector2f(diaPos.x, diaPos.y + 16.0f));       // Bottom point
    diamond.setPoint(1, sf::Vector2f(diaPos.x + 18.0f, diaPos.y));       // Right corner
    diamond.setPoint(2, sf::Vector2f(diaPos.x + 10.0f, diaPos.y - 14.0f)); // Top right
    diamond.setPoint(3, sf::Vector2f(diaPos.x - 10.0f, diaPos.y - 14.0f)); // Top left
    diamond.setPoint(4, sf::Vector2f(diaPos.x - 18.0f, diaPos.y));       // Left corner
    diamond.setFillColor(sf::Color(110, 225, 255));
    diamond.setOutlineColor(sf::Color(255, 255, 255));
    diamond.setOutlineThickness(1.8f);
    window.draw(diamond);

    // Diamond Glint Sparkle
    float glintPhase = std::abs(std::sin(m_stageTime * 4.0f));
    sf::CircleShape glint(5.0f * glintPhase);
    glint.setOrigin(sf::Vector2f(glint.getRadius(), glint.getRadius()));
    glint.setPosition(sf::Vector2f(diaPos.x - 6.0f, diaPos.y - 6.0f));
    glint.setFillColor(sf::Color(255, 255, 255, 230));
    window.draw(glint);

    // Massive Round Blown-Open Vault Blast Door (Hanging tilted off hinges!)
    sf::CircleShape vaultDoor(95.0f);
    vaultDoor.setOrigin(sf::Vector2f(95.0f, 95.0f));
    vaultDoor.setPosition(sf::Vector2f(vaultX - 165.0f, vaultY + 150.0f));
    vaultDoor.setFillColor(sf::Color(78, 85, 96));
    vaultDoor.setOutlineColor(sf::Color(32, 36, 44));
    vaultDoor.setOutlineThickness(4.5f);
    window.draw(vaultDoor);

    // Central Locking Wheel on the door
    sf::CircleShape wheel(32.0f);
    wheel.setOrigin(sf::Vector2f(32.0f, 32.0f));
    wheel.setPosition(vaultDoor.getPosition());
    wheel.setFillColor(sf::Color(45, 50, 60));
    wheel.setOutlineColor(sf::Color(255, 215, 0)); // Brass trim
    wheel.setOutlineThickness(2.5f);
    window.draw(wheel);
}

// -----------------------------------------------------------------------------
// 2. THEMED PLATFORMS PASS (WORLD-SPACE)
// -----------------------------------------------------------------------------
void StageRenderer::drawPlatforms(sf::RenderWindow& window, const std::vector<StaticPlatform>& platforms) {
    switch (m_currentStage) {
        case StageType::ToppatAirship:
            drawAirshipPlatforms(window, platforms);
            break;
        case StageType::TheWall:
            drawTheWallPlatforms(window, platforms);
            break;
        case StageType::BankVault:
            drawBankVaultPlatforms(window, platforms);
            break;
    }
}

void StageRenderer::drawAirshipPlatforms(sf::RenderWindow& window, const std::vector<StaticPlatform>& platforms) {
    for (const auto& plat : platforms) {
        bool isWall = (plat.heightPixels > 200.0f);
        bool isFloor = (plat.widthPixels > 1000.0f);

        // 1. Base Platform Body (High-tech dark industrial steel)
        sf::RectangleShape rect(sf::Vector2f(plat.widthPixels, plat.heightPixels));
        rect.setOrigin(sf::Vector2f(plat.widthPixels * 0.5f, plat.heightPixels * 0.5f));
        rect.setPosition(plat.positionPixels);
        rect.setFillColor(sf::Color(28, 31, 38));
        rect.setOutlineColor(sf::Color(65, 72, 88));
        rect.setOutlineThickness(2.2f);
        window.draw(rect);

        // 2. Floor Ring & Top Ledge (Toppat industrial safety)
        if (!isWall) {
            float topY = plat.positionPixels.y - plat.heightPixels * 0.5f;

            // Center Ring Octagon / Circle in perspective
            if (isFloor) {
                sf::CircleShape ring(90.0f);
                ring.setScale(sf::Vector2f(1.0f, 0.26f)); // Perspective floor ring
                ring.setOrigin(sf::Vector2f(90.0f, 90.0f));
                ring.setPosition(sf::Vector2f(plat.positionPixels.x, topY + 8.0f));
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineColor(sf::Color(245, 195, 35, 140)); // Golden Toppat ring
                ring.setOutlineThickness(3.0f);
                window.draw(ring);

                sf::CircleShape innerRing(45.0f);
                innerRing.setScale(sf::Vector2f(1.0f, 0.26f));
                innerRing.setOrigin(sf::Vector2f(45.0f, 45.0f));
                innerRing.setPosition(sf::Vector2f(plat.positionPixels.x, topY + 8.0f));
                innerRing.setFillColor(sf::Color(165, 24, 38, 90)); // Crimson center
                innerRing.setOutlineColor(sf::Color(245, 195, 35, 170));
                innerRing.setOutlineThickness(2.0f);
                window.draw(innerRing);
            }

            float stripeWidth = 16.0f;
            int numStripes = static_cast<int>(plat.widthPixels / stripeWidth);

            for (int s = 0; s < numStripes; ++s) {
                sf::RectangleShape stripe(sf::Vector2f(stripeWidth * 0.5f, 4.0f));
                stripe.setPosition(sf::Vector2f(plat.positionPixels.x - plat.widthPixels * 0.5f + s * stripeWidth, topY));
                stripe.setFillColor((s % 2 == 0) ? sf::Color(245, 195, 35) : sf::Color(25, 25, 30));
                window.draw(stripe);
            }

            // Glowing blue energy conduit line along underside
            sf::RectangleShape conduit(sf::Vector2f(plat.widthPixels - 12.0f, 2.5f));
            conduit.setOrigin(sf::Vector2f((plat.widthPixels - 12.0f) * 0.5f, 1.25f));
            conduit.setPosition(sf::Vector2f(plat.positionPixels.x, plat.positionPixels.y + plat.heightPixels * 0.5f - 1.0f));
            conduit.setFillColor(sf::Color(60, 180, 255, 180));
            window.draw(conduit);
        } else {
            // Bulkhead rivets on arena walls
            for (float ry = plat.positionPixels.y - plat.heightPixels * 0.45f; ry < plat.positionPixels.y + plat.heightPixels * 0.45f; ry += 50.0f) {
                sf::CircleShape rivet(3.0f);
                rivet.setOrigin(sf::Vector2f(3.0f, 3.0f));
                rivet.setPosition(sf::Vector2f(plat.positionPixels.x, ry));
                rivet.setFillColor(sf::Color(140, 150, 170));
                window.draw(rivet);
            }
        }
    }
}

void StageRenderer::drawTheWallPlatforms(sf::RenderWindow& window, const std::vector<StaticPlatform>& platforms) {
    for (const auto& plat : platforms) {
        bool isWall = (plat.heightPixels > 200.0f);
        bool isFloor = (plat.widthPixels > 1000.0f);

        // 1. Reinforced Weathered Concrete Body
        sf::RectangleShape rect(sf::Vector2f(plat.widthPixels, plat.heightPixels));
        rect.setOrigin(sf::Vector2f(plat.widthPixels * 0.5f, plat.heightPixels * 0.5f));
        rect.setPosition(plat.positionPixels);
        rect.setFillColor(sf::Color(38, 42, 48));
        rect.setOutlineColor(sf::Color(18, 20, 24));
        rect.setOutlineThickness(2.5f);
        window.draw(rect);

        // 2. Thick Snow Cap & Prison Yard Boundary Line
        if (!isWall) {
            float topY = plat.positionPixels.y - plat.heightPixels * 0.5f;

            if (isFloor) {
                // Red Painted Prison Yard Limit Line in perspective
                sf::RectangleShape redLine(sf::Vector2f(plat.widthPixels - 120.0f, 3.5f));
                redLine.setOrigin(sf::Vector2f((plat.widthPixels - 120.0f) * 0.5f, 1.75f));
                redLine.setPosition(sf::Vector2f(plat.positionPixels.x, topY + 7.0f));
                redLine.setFillColor(sf::Color(210, 40, 40, 160));
                window.draw(redLine);
            }

            sf::RectangleShape snow(sf::Vector2f(plat.widthPixels + 4.0f, 6.0f));
            snow.setOrigin(sf::Vector2f((plat.widthPixels + 4.0f) * 0.5f, 5.0f));
            snow.setPosition(sf::Vector2f(plat.positionPixels.x, topY));
            snow.setFillColor(sf::Color(235, 242, 252));
            snow.setOutlineColor(sf::Color(180, 195, 215));
            snow.setOutlineThickness(1.0f);
            window.draw(snow);
        } else {
            // Prison guard floodlights mounted on arena side walls
            sf::CircleShape lamp(8.0f);
            lamp.setOrigin(sf::Vector2f(8.0f, 8.0f));
            lamp.setPosition(sf::Vector2f(plat.positionPixels.x, plat.positionPixels.y - 120.0f));
            lamp.setFillColor(sf::Color(255, 245, 180));
            lamp.setOutlineColor(sf::Color(40, 45, 55));
            lamp.setOutlineThickness(2.0f);
            window.draw(lamp);
        }
    }
}

void StageRenderer::drawBankVaultPlatforms(sf::RenderWindow& window, const std::vector<StaticPlatform>& platforms) {
    for (const auto& plat : platforms) {
        bool isWall = (plat.heightPixels > 200.0f);
        bool isFloor = (plat.widthPixels > 1000.0f);

        // 1. Weathered Desert Sandstone / Heavy Steel Vault Slabs
        sf::RectangleShape rect(sf::Vector2f(plat.widthPixels, plat.heightPixels));
        rect.setOrigin(sf::Vector2f(plat.widthPixels * 0.5f, plat.heightPixels * 0.5f));
        rect.setPosition(plat.positionPixels);
        rect.setFillColor(sf::Color(135, 68, 42)); // Sandstone
        rect.setOutlineColor(sf::Color(75, 32, 18));
        rect.setOutlineThickness(2.5f);
        window.draw(rect);

        // 2. Polished Brass / Gold Trim on Top Ledges
        if (!isWall) {
            float topY = plat.positionPixels.y - plat.heightPixels * 0.5f;

            if (isFloor) {
                // Bank Vault Yellow Safety Clearance Ring
                sf::CircleShape vaultRing(85.0f);
                vaultRing.setScale(sf::Vector2f(1.0f, 0.26f));
                vaultRing.setOrigin(sf::Vector2f(85.0f, 85.0f));
                vaultRing.setPosition(sf::Vector2f(plat.positionPixels.x, topY + 8.0f));
                vaultRing.setFillColor(sf::Color::Transparent);
                vaultRing.setOutlineColor(sf::Color(255, 215, 0, 150));
                vaultRing.setOutlineThickness(3.0f);
                window.draw(vaultRing);
            }

            sf::RectangleShape trim(sf::Vector2f(plat.widthPixels, 3.5f));
            trim.setOrigin(sf::Vector2f(plat.widthPixels * 0.5f, 1.75f));
            trim.setPosition(sf::Vector2f(plat.positionPixels.x, topY));
            trim.setFillColor(sf::Color(235, 180, 50));
            window.draw(trim);
        }
    }
}

// -----------------------------------------------------------------------------
// 3. WALL DAMAGE DECALS PASS
// -----------------------------------------------------------------------------
void StageRenderer::drawWallCracks(sf::RenderWindow& window) {
    for (const auto& crack : m_wallCracks) {
        float alphaProg = crack.lifeTime / crack.maxLifeTime;
        std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(alphaProg * 255.0f, 0.0f, 255.0f));
        sf::Color crackColor(25, 25, 30, alpha);

        for (const auto& branch : crack.branches) {
            for (size_t i = 1; i < branch.size(); ++i) {
                sf::Vector2f p1 = branch[i - 1];
                sf::Vector2f p2 = branch[i];
                sf::Vector2f delta = p2 - p1;
                float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                if (len < 0.5f) continue;

                sf::RectangleShape line(sf::Vector2f(len, 2.4f));
                line.setOrigin(sf::Vector2f(0.0f, 1.2f));
                line.setPosition(p1);
                line.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
                line.setFillColor(crackColor);
                window.draw(line);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// 4. ATMOSPHERIC OVERLAYS & DYNAMIC LIGHTING PASS (DRAWN OVER FIGHTERS/STAGE)
// -----------------------------------------------------------------------------
void StageRenderer::drawAtmosphereAndLighting(sf::RenderWindow& window, const sf::View& cameraView) {
    (void)cameraView;

    // A. The Wall Sweeping Volumetric Searchlights
    if (m_currentStage == StageType::TheWall) {
        // Spotlight 1 (Left Guard Post at X=160, Y=180)
        sf::Vector2f src1(160.0f, 180.0f);
        float rad1 = (90.0f + m_searchlightAngle1) * 0.017453f;
        float beamLen1 = 850.0f;
        float spread1 = 0.22f; // Cone angle spread

        sf::VertexArray beam1(sf::PrimitiveType::Triangles);
        sf::Color coreColor(255, 250, 205, 50);
        sf::Color edgeColor(255, 250, 205, 0);

        sf::Vector2f end1A = src1 + sf::Vector2f(std::cos(rad1 - spread1) * beamLen1, std::sin(rad1 - spread1) * beamLen1);
        sf::Vector2f end1B = src1 + sf::Vector2f(std::cos(rad1 + spread1) * beamLen1, std::sin(rad1 + spread1) * beamLen1);

        beam1.append(sf::Vertex{ src1, coreColor });
        beam1.append(sf::Vertex{ end1A, edgeColor });
        beam1.append(sf::Vertex{ end1B, edgeColor });
        window.draw(beam1);

        // Spotlight 2 (Right Guard Post at X=1440, Y=180)
        sf::Vector2f src2(1440.0f, 180.0f);
        float rad2 = (90.0f + m_searchlightAngle2) * 0.017453f;
        float beamLen2 = 850.0f;
        float spread2 = 0.22f;

        sf::VertexArray beam2(sf::PrimitiveType::Triangles);
        sf::Vector2f end2A = src2 + sf::Vector2f(std::cos(rad2 - spread2) * beamLen2, std::sin(rad2 - spread2) * beamLen2);
        sf::Vector2f end2B = src2 + sf::Vector2f(std::cos(rad2 + spread2) * beamLen2, std::sin(rad2 + spread2) * beamLen2);

        beam2.append(sf::Vertex{ src2, coreColor });
        beam2.append(sf::Vertex{ end2A, edgeColor });
        beam2.append(sf::Vertex{ end2B, edgeColor });
        window.draw(beam2);

        // Falling Snow Blizzard Particles
        for (const auto& p : m_snowflakes) {
            sf::CircleShape flake(p.size * 0.5f);
            flake.setPosition(p.pos);
            flake.setFillColor(p.color);
            window.draw(flake);
        }
    }
    // B. Airship High-Altitude Wind Streaks
    else if (m_currentStage == StageType::ToppatAirship) {
        for (const auto& p : m_airshipWindStreaks) {
            sf::RectangleShape streak(sf::Vector2f(p.size, 1.8f));
            streak.setPosition(p.pos);
            streak.setFillColor(p.color);
            window.draw(streak);
        }
    }
    // C. Desert Dust & Rolling Tumbleweed
    else if (m_currentStage == StageType::BankVault) {
        for (const auto& p : m_desertDust) {
            sf::CircleShape dust(p.size * 0.5f);
            dust.setPosition(p.pos);
            dust.setFillColor(p.color);
            window.draw(dust);
        }

        // Rolling Tumbleweed
        if (m_tumbleweedPos.x > -80.0f && m_tumbleweedPos.x < 1680.0f) {
            sf::CircleShape weed(16.0f);
            weed.setOrigin(sf::Vector2f(16.0f, 16.0f));
            weed.setPosition(m_tumbleweedPos);
            weed.setRotation(sf::degrees(m_tumbleweedRot));
            weed.setFillColor(sf::Color::Transparent);
            weed.setOutlineColor(sf::Color(165, 125, 75));
            weed.setOutlineThickness(2.2f);
            window.draw(weed);

            // Internal branch spokes
            for (int k = 0; k < 4; ++k) {
                sf::RectangleShape branch(sf::Vector2f(30.0f, 2.0f));
                branch.setOrigin(sf::Vector2f(15.0f, 1.0f));
                branch.setPosition(m_tumbleweedPos);
                branch.setRotation(sf::degrees(m_tumbleweedRot + k * 45.0f));
                branch.setFillColor(sf::Color(145, 105, 55));
                window.draw(branch);
            }
        }
    }

    // D. Ground Landing / Footstep Dust Puffs
    for (const auto& p : m_groundDust) {
        float alphaProg = p.life / p.maxLife;
        sf::Color col = p.color;
        col.a = static_cast<std::uint8_t>(col.a * alphaProg);

        sf::CircleShape puff(p.size);
        puff.setOrigin(sf::Vector2f(p.size, p.size));
        puff.setPosition(p.pos);
        puff.setFillColor(col);
        window.draw(puff);
    }
}

} // namespace RagdollEngine
