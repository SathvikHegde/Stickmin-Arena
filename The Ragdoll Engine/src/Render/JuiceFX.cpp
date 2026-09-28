#include "RagdollEngine/Render/JuiceFX.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Text.hpp>
#include <cmath>
#include <random>
#include <algorithm>
#include <cstdint>

namespace RagdollEngine {

JuiceFX::JuiceFX() = default;

void JuiceFX::spawnImpact(const sf::Vector2f& pos, const sf::Vector2f& impactDir, const sf::Color& color, bool heavy) {
    static std::mt19937 rng(1337);

    // 1. Shockwave Ring
    Shockwave sw;
    sw.position = pos;
    sw.currentRadius = 8.0f;
    sw.maxRadius = heavy ? 110.0f : 65.0f;
    sw.lifeTime = heavy ? 0.30f : 0.22f;
    sw.maxLifeTime = sw.lifeTime;
    sw.color = color;
    m_shockwaves.push_back(sw);

    // 2. Spiky Impact Starburst
    ImpactStarburst sb;
    sb.position = pos;
    sb.currentScale = 0.2f;
    sb.maxScale = heavy ? 1.8f : 1.0f;
    std::uniform_real_distribution<float> rotDist(0.0f, 90.0f);
    sb.rotationDeg = rotDist(rng);
    sb.lifeTime = heavy ? 0.22f : 0.16f;
    sb.maxLifeTime = sb.lifeTime;
    sb.color = sf::Color(255, 250, 200);
    m_starbursts.push_back(sb);

    // 3. Directional Sparks
    std::uniform_real_distribution<float> angleSpread(-0.9f, 0.9f);
    std::uniform_real_distribution<float> speedDist(heavy ? 250.0f : 180.0f, heavy ? 650.0f : 450.0f);

    float baseAngle = std::atan2(impactDir.y, impactDir.x);
    int sparkCount = heavy ? 18 : 9;
    for (int i = 0; i < sparkCount; ++i) {
        HitSpark spark;
        spark.position = pos;
        float angle = baseAngle + angleSpread(rng);
        float speed = speedDist(rng);
        spark.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
        spark.lifeTime = heavy ? 0.25f : 0.18f;
        spark.maxLifeTime = spark.lifeTime;
        spark.length = 14.0f + (speed * 0.035f);
        spark.color = color;
        m_sparks.push_back(spark);
    }
}

void JuiceFX::spawnBlockEffect(const sf::Vector2f& pos) {
    BlockBarrier barrier;
    barrier.position = pos;
    barrier.radius = 26.0f;
    barrier.lifeTime = 0.22f;
    barrier.maxLifeTime = 0.22f;
    barrier.color = sf::Color(90, 210, 255);
    m_blocks.push_back(barrier);

    // Small blue block spark scatter
    static std::mt19937 rng(777);
    std::uniform_real_distribution<float> angleDist(-3.14159f, 3.14159f);
    std::uniform_real_distribution<float> speedDist(80.0f, 220.0f);
    for (int i = 0; i < 6; ++i) {
        HitSpark spark;
        spark.position = pos;
        float angle = angleDist(rng);
        float speed = speedDist(rng);
        spark.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
        spark.lifeTime = 0.14f;
        spark.maxLifeTime = 0.14f;
        spark.length = 8.0f;
        spark.color = sf::Color(140, 220, 255);
        m_sparks.push_back(spark);
    }
}

void JuiceFX::spawnElectricBurst(const sf::Vector2f& center, const sf::Color& color, int boltCount, float radius) {
    static std::mt19937 rng(999);
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> radDist(radius * 0.4f, radius * 1.2f);
    std::uniform_real_distribution<float> jitterDist(-10.0f, 10.0f);

    for (int i = 0; i < boltCount; ++i) {
        float angle = angleDist(rng);
        float r = radDist(rng);
        sf::Vector2f target = center + sf::Vector2f(std::cos(angle) * r, std::sin(angle) * r);
        sf::Vector2f mid = (center + target) * 0.5f + sf::Vector2f(jitterDist(rng), jitterDist(rng));

        ElectricSegment seg1;
        seg1.p1 = center;
        seg1.p2 = mid;
        seg1.lifeTime = 0.12f;
        seg1.maxLifeTime = 0.12f;
        seg1.color = color;
        m_electricArcs.push_back(seg1);

        ElectricSegment seg2;
        seg2.p1 = mid;
        seg2.p2 = target;
        seg2.lifeTime = 0.12f;
        seg2.maxLifeTime = 0.12f;
        seg2.color = sf::Color::White;
        m_electricArcs.push_back(seg2);
    }
}

void JuiceFX::spawnFloatingText(const sf::Vector2f& pos, const std::string& text, const sf::Color& color, float scale) {
    FloatingTextItem item;
    item.position = pos;
    item.velocity = sf::Vector2f(0.0f, -65.0f);
    item.text = text;
    item.lifeTime = 0.70f;
    item.maxLifeTime = 0.70f;
    item.color = color;
    item.scale = scale;
    m_floatingTexts.push_back(item);
}

void JuiceFX::addTrailPoint(int trailSlot, const sf::Vector2f& point, const sf::Color& color, float width) {
    if (trailSlot < 0 || trailSlot >= 8) return;
    TrailSegment seg;
    seg.pos = point;
    seg.width = width;
    seg.lifeTime = 0.14f;
    seg.maxLifeTime = 0.14f;
    seg.color = color;
    m_trails[trailSlot].push_back(seg);

    // Limit trail history length
    if (m_trails[trailSlot].size() > 16) {
        m_trails[trailSlot].erase(m_trails[trailSlot].begin());
    }
}

void JuiceFX::triggerScreenFlash(float duration, const sf::Color& color) {
    m_screenFlashTimer = duration;
    m_screenFlashDuration = duration;
    m_screenFlashColor = color;
}

void JuiceFX::update(float dt) {
    // 1. Shockwaves
    for (auto it = m_shockwaves.begin(); it != m_shockwaves.end();) {
        it->lifeTime -= dt;
        if (it->lifeTime <= 0.0f) {
            it = m_shockwaves.erase(it);
        } else {
            float progress = 1.0f - (it->lifeTime / it->maxLifeTime);
            it->currentRadius = 8.0f + (it->maxRadius - 8.0f) * progress;
            ++it;
        }
    }

    // 2. Starbursts
    for (auto it = m_starbursts.begin(); it != m_starbursts.end();) {
        it->lifeTime -= dt;
        if (it->lifeTime <= 0.0f) {
            it = m_starbursts.erase(it);
        } else {
            float progress = 1.0f - (it->lifeTime / it->maxLifeTime);
            it->currentScale = 0.2f + (it->maxScale - 0.2f) * std::sin(progress * 1.57f);
            it->rotationDeg += dt * 360.0f;
            ++it;
        }
    }

    // 3. Sparks
    for (auto it = m_sparks.begin(); it != m_sparks.end();) {
        it->lifeTime -= dt;
        if (it->lifeTime <= 0.0f) {
            it = m_sparks.erase(it);
        } else {
            it->position += it->velocity * dt;
            it->velocity *= (1.0f - 4.5f * dt);
            ++it;
        }
    }

    // 4. Electric arcs
    for (auto it = m_electricArcs.begin(); it != m_electricArcs.end();) {
        it->lifeTime -= dt;
        if (it->lifeTime <= 0.0f) {
            it = m_electricArcs.erase(it);
        } else {
            ++it;
        }
    }

    // 5. Block barriers
    for (auto it = m_blocks.begin(); it != m_blocks.end();) {
        it->lifeTime -= dt;
        if (it->lifeTime <= 0.0f) {
            it = m_blocks.erase(it);
        } else {
            ++it;
        }
    }

    // 6. Floating text
    for (auto it = m_floatingTexts.begin(); it != m_floatingTexts.end();) {
        it->lifeTime -= dt;
        if (it->lifeTime <= 0.0f) {
            it = m_floatingTexts.erase(it);
        } else {
            it->position += it->velocity * dt;
            it->velocity.y += 20.0f * dt; // Slow down drift
            ++it;
        }
    }

    // 7. Motion trails
    for (int s = 0; s < 8; ++s) {
        for (auto it = m_trails[s].begin(); it != m_trails[s].end();) {
            it->lifeTime -= dt;
            if (it->lifeTime <= 0.0f) {
                it = m_trails[s].erase(it);
            } else {
                ++it;
            }
        }
    }

    // 8. Screen flash
    if (m_screenFlashTimer > 0.0f) {
        m_screenFlashTimer -= dt;
    }
}

void JuiceFX::draw(sf::RenderWindow& window) {
    // 1. Draw Motion Trails (Fading ribbon strips)
    for (int s = 0; s < 8; ++s) {
        const auto& trail = m_trails[s];
        if (trail.size() < 2) continue;

        sf::VertexArray strip(sf::PrimitiveType::TriangleStrip);
        for (size_t i = 0; i < trail.size(); ++i) {
            float progress = trail[i].lifeTime / trail[i].maxLifeTime;
            std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(progress * 220.0f, 0.0f, 255.0f));
            sf::Color segColor = trail[i].color;
            segColor.a = alpha;

            // Direction to next or prev
            sf::Vector2f dir(1.0f, 0.0f);
            if (i + 1 < trail.size()) {
                dir = trail[i + 1].pos - trail[i].pos;
            } else if (i > 0) {
                dir = trail[i].pos - trail[i - 1].pos;
            }
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.001f) dir /= len;
            sf::Vector2f norm(-dir.y, dir.x);

            float halfW = trail[i].width * 0.5f * progress;
            sf::Vertex v1;
            v1.position = trail[i].pos + norm * halfW;
            v1.color = segColor;

            sf::Vertex v2;
            v2.position = trail[i].pos - norm * halfW;
            v2.color = segColor;

            strip.append(v1);
            strip.append(v2);
        }
        window.draw(strip);
    }

    // 2. Draw Shockwaves
    for (const auto& sw : m_shockwaves) {
        float alphaProgress = sw.lifeTime / sw.maxLifeTime;
        std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(alphaProgress * 255.0f, 0.0f, 255.0f));

        sf::CircleShape ring(sw.currentRadius);
        ring.setOrigin(sf::Vector2f(sw.currentRadius, sw.currentRadius));
        ring.setPosition(sw.position);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(sf::Color(sw.color.r, sw.color.g, sw.color.b, alpha));
        ring.setOutlineThickness(4.0f * alphaProgress);
        window.draw(ring);
    }

    // 3. Draw Impact Starbursts (Anime/Comic 4-point diamond star)
    for (const auto& sb : m_starbursts) {
        float alphaProgress = sb.lifeTime / sb.maxLifeTime;
        std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(alphaProgress * 255.0f, 0.0f, 255.0f));
        sf::Color col = sb.color;
        col.a = alpha;

        float outer = 32.0f * sb.currentScale;
        float inner = 7.0f * sb.currentScale;

        sf::ConvexShape star(8);
        for (int i = 0; i < 8; ++i) {
            float r = (i % 2 == 0) ? outer : inner;
            float rad = (i * 45.0f + sb.rotationDeg) * 3.14159f / 180.0f;
            star.setPoint(i, sf::Vector2f(std::cos(rad) * r, std::sin(rad) * r));
        }
        star.setPosition(sb.position);
        star.setFillColor(col);
        window.draw(star);
    }

    // 4. Draw Electric Lightning Arcs
    if (!m_electricArcs.empty()) {
        sf::VertexArray lines(sf::PrimitiveType::Lines);
        for (const auto& arc : m_electricArcs) {
            float progress = arc.lifeTime / arc.maxLifeTime;
            std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(progress * 255.0f, 0.0f, 255.0f));
            sf::Color col = arc.color;
            col.a = alpha;

            sf::Vertex v1;
            v1.position = arc.p1;
            v1.color = col;

            sf::Vertex v2;
            v2.position = arc.p2;
            v2.color = sf::Color(255, 255, 255, alpha);

            lines.append(v1);
            lines.append(v2);
        }
        window.draw(lines);
    }

    // 5. Draw Block Barriers (Hexagonal shield)
    for (const auto& b : m_blocks) {
        float alphaProgress = b.lifeTime / b.maxLifeTime;
        std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(alphaProgress * 255.0f, 0.0f, 255.0f));

        sf::CircleShape hex(b.radius, 6);
        hex.setOrigin(sf::Vector2f(b.radius, b.radius));
        hex.setPosition(b.position);
        hex.setFillColor(sf::Color(b.color.r, b.color.g, b.color.b, static_cast<std::uint8_t>(alpha * 0.35f)));
        hex.setOutlineColor(sf::Color(255, 255, 255, alpha));
        hex.setOutlineThickness(2.5f);
        window.draw(hex);
    }

    // 6. Draw Directional Sparks
    for (const auto& spark : m_sparks) {
        float alphaProgress = spark.lifeTime / spark.maxLifeTime;
        std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(alphaProgress * 255.0f, 0.0f, 255.0f));

        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        line[0].position = spark.position;
        line[0].color = sf::Color(spark.color.r, spark.color.g, spark.color.b, alpha);

        sf::Vector2f dir = spark.velocity;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.001f) dir /= len;

        line[1].position = spark.position - dir * spark.length;
        line[1].color = sf::Color(spark.color.r, spark.color.g, spark.color.b, 0);
        window.draw(line);
    }

    // 7. Draw Floating Combat Text
    if (m_font) {
        for (const auto& item : m_floatingTexts) {
            float progress = item.lifeTime / item.maxLifeTime;
            std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(progress * 255.0f, 0.0f, 255.0f));

            sf::Text text(*m_font, item.text, static_cast<unsigned int>(18.0f * item.scale));
            text.setStyle(sf::Text::Bold);
            text.setFillColor(sf::Color(item.color.r, item.color.g, item.color.b, alpha));
            text.setOutlineColor(sf::Color(0, 0, 0, alpha));
            text.setOutlineThickness(2.0f);
            text.setOrigin(sf::Vector2f(text.getLocalBounds().size.x * 0.5f, text.getLocalBounds().size.y * 0.5f));
            text.setPosition(item.position);
            window.draw(text);
        }
    }

    // 8. Screen Flash
    if (m_screenFlashTimer > 0.0f) {
        float flashProgress = m_screenFlashTimer / m_screenFlashDuration;
        std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(m_screenFlashColor.a * flashProgress, 0.0f, 255.0f));

        sf::View currentView = window.getView();
        sf::RectangleShape overlay(currentView.getSize());
        overlay.setOrigin(currentView.getSize() * 0.5f);
        overlay.setPosition(currentView.getCenter());
        overlay.setFillColor(sf::Color(m_screenFlashColor.r, m_screenFlashColor.g, m_screenFlashColor.b, alpha));
        window.draw(overlay);
    }
}

} // namespace RagdollEngine
