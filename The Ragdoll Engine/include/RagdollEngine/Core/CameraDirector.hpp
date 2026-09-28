#pragma once
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <vector>

namespace RagdollEngine {

class CameraDirector {
public:
    CameraDirector(float viewportWidth, float viewportHeight);

    void addFocusPoint(const sf::Vector2f& point);
    void clearFocusPoints();

    // Trauma-based screen shake (0.0 to 1.0)
    void addTrauma(float amount);

    // Update camera position, zoom, and shake
    void update(float realDt);

    // Apply view to the SFML render window
    void apply(sf::RenderWindow& window);

    // Accessors
    const sf::View& getView() const { return m_view; }
    void setBaseCenter(const sf::Vector2f& center) { m_targetCenter = center; }
    void setZoomLimits(float minZoom, float maxZoom) { m_minZoom = minZoom; m_maxZoom = maxZoom; }

    // Cinematic Clashes & Finishers
    void triggerCinematicZoom(float targetZoom, float durationSeconds, float tiltAngleDeg = 0.0f);
    bool isCinematicActive() const { return m_cinematicTimer > 0.0f; }

private:
    sf::View m_view;
    sf::Vector2f m_viewportSize;

    std::vector<sf::Vector2f> m_focusPoints;
    sf::Vector2f m_currentCenter;
    sf::Vector2f m_targetCenter;

    float m_currentZoom{ 1.0f };
    float m_targetZoom{ 1.0f };
    float m_minZoom{ 0.6f };  // Close up
    float m_maxZoom{ 1.6f };  // Far wide

    // Cinematic Overrides
    float m_cinematicTimer{ 0.0f };
    float m_cinematicDuration{ 0.0f };
    float m_cinematicZoom{ 0.70f };
    float m_cinematicTilt{ 0.0f };

    // Screen Shake (Trauma model: shake = trauma^2)
    float m_trauma{ 0.0f };
    float m_traumaDecay{ 1.6f };
    float m_maxOffsetPixels{ 25.0f };
    float m_maxAngleDeg{ 4.0f };

    float m_shakeOffsetX{ 0.0f };
    float m_shakeOffsetY{ 0.0f };
    float m_shakeAngle{ 0.0f };

    // Random generator helper
    float getRandomFloat(float min, float max);
};

} // namespace RagdollEngine
