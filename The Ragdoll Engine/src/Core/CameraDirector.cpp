#include "RagdollEngine/Core/CameraDirector.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace RagdollEngine {

CameraDirector::CameraDirector(float viewportWidth, float viewportHeight)
    : m_viewportSize(viewportWidth, viewportHeight),
      m_currentCenter(viewportWidth * 0.5f, viewportHeight * 0.5f),
      m_targetCenter(viewportWidth * 0.5f, viewportHeight * 0.5f) {
    m_view.setSize(sf::Vector2f(viewportWidth, viewportHeight));
    m_view.setCenter(m_currentCenter);
}

void CameraDirector::addFocusPoint(const sf::Vector2f& point) {
    m_focusPoints.push_back(point);
}

void CameraDirector::clearFocusPoints() {
    m_focusPoints.clear();
}

void CameraDirector::addTrauma(float amount) {
    m_trauma = std::min(1.0f, m_trauma + amount);
}

float CameraDirector::getRandomFloat(float min, float max) {
    static std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

void CameraDirector::triggerCinematicZoom(float targetZoom, float durationSeconds, float tiltAngleDeg) {
    m_cinematicZoom = std::clamp(targetZoom, 0.22f, m_maxZoom);
    m_cinematicDuration = durationSeconds;
    m_cinematicTimer = durationSeconds;
    m_cinematicTilt = tiltAngleDeg;
}

void CameraDirector::update(float realDt) {
    // 1. Calculate target center & required zoom from focus points
    if (!m_focusPoints.empty()) {
        float minX = m_focusPoints[0].x;
        float maxX = m_focusPoints[0].x;
        float minY = m_focusPoints[0].y;
        float maxY = m_focusPoints[0].y;

        for (const auto& pt : m_focusPoints) {
            minX = std::min(minX, pt.x);
            maxX = std::max(maxX, pt.x);
            minY = std::min(minY, pt.y);
            maxY = std::max(maxY, pt.y);
        }

        // Horizontal midpoint
        float midX = (minX + maxX) * 0.5f;

        // Ground anchor: the floor is at Y = 800. Fighters stand at Y ~ 735.
        // Base camera center Y is 710.0f, which places the floor in the lower ~25% of the viewport.
        // High jumps smoothly pan the camera up proportionally without losing the floor.
        float highestY = std::min(minY, 715.0f);
        float jumpPan = (715.0f - highestY) * 0.40f;
        float targetY = 710.0f - jumpPan;

        m_targetCenter = sf::Vector2f(midX, targetY);

        // Dynamic Tekken zoom calculation to guarantee both fighters stay comfortably in frame:
        // Horizontal span with 140px margin on each side (total +280px)
        float spanX = (maxX - minX) + 280.0f;
        // Vertical span with 120px margin above and below (total +240px)
        float spanY = (maxY - minY) + 240.0f;

        float requiredZoomX = spanX / m_viewportSize.x;
        float requiredZoomY = spanY / m_viewportSize.y;
        float desiredZoom = std::max(requiredZoomX, requiredZoomY);
        m_targetZoom = std::clamp(desiredZoom, m_minZoom, m_maxZoom);

        // Arena boundary soft clamp (arena walls are at X = 60 and X = 1540)
        float halfW = m_viewportSize.x * m_targetZoom * 0.5f;
        float minCamX = 60.0f + halfW;
        float maxCamX = 1540.0f - halfW;
        if (minCamX <= maxCamX) {
            // View fits within arena: clamp camera center so we don't look past the walls,
            // but ensure fighters never get pushed off screen.
            float clampedX = std::clamp(midX, minCamX, maxCamX);
            if (minX >= clampedX - halfW + 40.0f && maxX <= clampedX + halfW - 40.0f) {
                m_targetCenter.x = clampedX;
            } else {
                m_targetCenter.x = midX;
            }
        } else {
            // View is wider than arena: center on arena midpoint (800)
            m_targetCenter.x = 800.0f;
        }
    }

    // Process Cinematic Override
    float desiredZoom = m_targetZoom;
    float currentTilt = 0.0f;
    if (m_cinematicTimer > 0.0f) {
        m_cinematicTimer -= realDt;
        float progress = m_cinematicTimer / m_cinematicDuration;
        desiredZoom = m_cinematicZoom * (1.0f - progress * 0.12f);
        currentTilt = m_cinematicTilt * progress;
    }

    // 2. Smoothly interpolate position and zoom
    float posLerp = (m_cinematicTimer > 0.0f ? 12.0f : 8.0f) * realDt;
    float zoomLerp = (m_cinematicTimer > 0.0f ? 10.0f : 6.0f) * realDt;
    m_currentCenter.x += (m_targetCenter.x - m_currentCenter.x) * std::min(1.0f, posLerp);
    m_currentCenter.y += (m_targetCenter.y - m_currentCenter.y) * std::min(1.0f, posLerp);
    m_currentZoom += (desiredZoom - m_currentZoom) * std::min(1.0f, zoomLerp);

    // 3. Process Trauma Screen Shake (shake = trauma^2)
    m_shakeOffsetX = 0.0f;
    m_shakeOffsetY = 0.0f;
    m_shakeAngle = currentTilt;

    if (m_trauma > 0.001f) {
        float shakeIntensity = m_trauma * m_trauma;
        m_shakeOffsetX = m_maxOffsetPixels * shakeIntensity * getRandomFloat(-1.0f, 1.0f);
        m_shakeOffsetY = m_maxOffsetPixels * shakeIntensity * getRandomFloat(-1.0f, 1.0f);
        m_shakeAngle += m_maxAngleDeg * shakeIntensity * getRandomFloat(-1.0f, 1.0f);

        // Decay trauma linearly over time
        m_trauma = std::max(0.0f, m_trauma - m_traumaDecay * realDt);
    }

    // 4. Update the SFML view
    m_view.setSize(sf::Vector2f(m_viewportSize.x * m_currentZoom, m_viewportSize.y * m_currentZoom));
    m_view.setCenter(sf::Vector2f(m_currentCenter.x + m_shakeOffsetX, m_currentCenter.y + m_shakeOffsetY));
    m_view.setRotation(sf::degrees(m_shakeAngle));
    m_view.setViewport(m_viewportRect);
}

void CameraDirector::apply(sf::RenderWindow& window) {
    m_view.setViewport(m_viewportRect);
    window.setView(m_view);
}

} // namespace RagdollEngine
