#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <RagdollEngine/Core/TimeManager.hpp>
#include <RagdollEngine/Render/RagdollRenderer.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>

#include "NetworkManager.hpp"
#include "MenuManager.hpp"
#include "SceneManager.hpp"
#include "GameContext.hpp"

#include <iostream>

int main() {
    // 1. Create Window with SFML 3 VideoMode
    sf::ContextSettings settings;
    sf::RenderWindow window(
        sf::VideoMode({1600, 900}),
        "Stickmin Arena",
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );
    window.setFramerateLimit(144);

    // 2. Initialize Engine Subsystems
    RagdollEngine::TimeManager timeManager;
    RagdollEngine::RagdollRenderer ragdollRenderer;
    RagdollEngine::StageRenderer stageRenderer;

    // Load Font
    sf::Font font;
    bool fontLoaded = font.openFromFile("C:/Windows/Fonts/segoeui.ttf") ||
                      font.openFromFile("C:/Windows/Fonts/arial.ttf");

    StickminGame::NetworkManager netManager;
    StickminGame::MenuManager menuManager(fontLoaded ? &font : nullptr, &ragdollRenderer, &stageRenderer, &netManager);

    // Resolution-independent virtual coordinate viewport (1600x900)
    sf::View uiView(sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(1600.0f, 900.0f)));
    auto updateLetterbox = [&](const sf::Vector2u& winSize) {
        float winW = static_cast<float>(winSize.x);
        float winH = static_cast<float>(winSize.y);
        if (winH <= 0.0f) winH = 1.0f;
        float winRatio = winW / winH;
        float targetRatio = 1600.0f / 900.0f; // 16:9

        float vpW = 1.0f;
        float vpH = 1.0f;
        float vpX = 0.0f;
        float vpY = 0.0f;

        if (winRatio >= targetRatio) {
            // Window is wider than 16:9 -> pillarbox (black bars on left/right)
            vpW = targetRatio / winRatio;
            vpX = (1.0f - vpW) * 0.5f;
        } else {
            // Window is taller than 16:9 -> letterbox (black bars on top/bottom)
            vpH = winRatio / targetRatio;
            vpY = (1.0f - vpH) * 0.5f;
        }

        sf::FloatRect vp(sf::Vector2f(vpX, vpY), sf::Vector2f(vpW, vpH));
        uiView.setSize(sf::Vector2f(1600.0f, 900.0f));
        uiView.setCenter(sf::Vector2f(800.0f, 450.0f));
        uiView.setViewport(vp);
    };
    updateLetterbox(window.getSize());

#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif

    (void)window.setActive(true);
    const char* glVendor   = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* glVersion  = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glslVer    = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    const auto& actualSettings = window.getSettings();

    std::cout << "====================================================\n";
    std::cout << " Stickmin Arena\n";
    std::cout << "====================================================\n";
    std::cout << " [Graphics Device & Driver]\n";
    std::cout << "   GPU / Renderer  : " << (glRenderer ? glRenderer : "Unknown") << "\n";
    std::cout << "   Driver / Vendor : " << (glVendor ? glVendor : "Unknown") << "\n";
    std::cout << "   OpenGL Version  : " << (glVersion ? glVersion : "Unknown") << "\n";
    if (glslVer) {
        std::cout << "   GLSL Version    : " << glslVer << "\n";
    }
    std::cout << "   Context Profile : OpenGL " << actualSettings.majorVersion << "." << actualSettings.minorVersion << "\n";
    std::cout << "   Depth / Stencil : " << actualSettings.depthBits << " / " << actualSettings.stencilBits << " bits\n";
    std::cout << "   MSAA Level      : " << actualSettings.antiAliasingLevel << "x\n";
    std::cout << "====================================================\n";

    // 3. Initialize Scene Architecture
    StickminGame::SceneManager sceneManager;
    StickminGame::GameContext context{
        window,
        timeManager,
        ragdollRenderer,
        stageRenderer,
        netManager,
        menuManager,
        fontLoaded ? &font : nullptr,
        uiView,
        sceneManager
    };
    sceneManager.setContext(&context);
    sceneManager.setScene(StickminGame::SceneType::Menu);

    // 4. Main Game Loop
    while (window.isOpen()) {
        timeManager.update();
        float dt = timeManager.getRealDeltaTime();

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                updateLetterbox(resized->size);
            }
            sceneManager.handleEvent(*event);
        }

        sceneManager.update(dt);

        window.clear(sf::Color(10, 12, 16));
        sceneManager.render(window);
        window.display();
    }

    return 0;
}
