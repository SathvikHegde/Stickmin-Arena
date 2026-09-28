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
    m_cinematicZoom = std::clamp(targetZoom, m_minZoom, m_maxZoom);
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

        // Midpoint
        m_targetCenter = sf::Vector2f((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);

        // Required span with safety padding
        float spanX = (maxX - minX) + 400.0f;
        float spanY = (maxY - minY) + 300.0f;

        float zoomX = spanX / m_viewportSize.x;
        float zoomY = spanY / m_viewportSize.y;
        m_targetZoom = std::max(zoomX, zoomY);
        m_targetZoom = std::clamp(m_targetZoom, m_minZoom, m_maxZoom);
    }

    // Process Cinematic Override
    float desiredZoom = m_targetZoom;
    float currentTilt = 0.0f;
    if (m_cinematicTimer > 0.0f) {
        m_cinematicTimer -= realDt;
        float progress = m_cinematicTimer / m_cinematicDuration;
        desiredZoom = m_cinematicZoom * (1.0f - progress * 0.15f);
        currentTilt = m_cinematicTilt * progress;
    }

    // 2. Smoothly interpolate position and zoom
    float posLerp = (m_cinematicTimer > 0.0f ? 10.0f : 6.0f) * realDt;
    float zoomLerp = (m_cinematicTimer > 0.0f ? 8.0f : 5.0f) * realDt;
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
}

void CameraDirector::apply(sf::RenderWindow& window) {
    window.setView(m_view);
}

} // namespace RagdollEngine
