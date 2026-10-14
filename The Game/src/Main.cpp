#include <SFML/Graphics.hpp>
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

    std::cout << "====================================================\n";
    std::cout << " Stickmin Arena - Tekken 7 Active Ragdolls!\n";
    std::cout << " Roster: Henry, Ellie, Charles Calvin, Reginald, RHM!\n";
    std::cout << " Player 1 (Tekken 7 4-Button Controls):\n";
    std::cout << "   A / D       : Move / Guard (Hold Back to Block!)\n";
    std::cout << "   S           : Crouch (Hold Down+Back for Crouch Block!)\n";
    std::cout << "   W / Space   : Jump\n";
    std::cout << "   J           : 1 (Left Punch - Flash Jab)\n";
    std::cout << "   K           : 2 (Right Punch - Straight Cross / Fwd+2: EWGF / 1,2: One-Two!)\n";
    std::cout << "   U           : 3 (Left Kick - Mid Kick / Down+3: Hell Sweep!)\n";
    std::cout << "   I           : 4 (Right Kick - Axe Roundhouse / Up+4: Hopkick!)\n";
    std::cout << "   O           : 1+2 (Power Crush Armor / Rage Art when in Rage!)\n";
    std::cout << "   L           : 1+3 (Command Throw - Unblockable Grab! Break with 1 or 2)\n";
    std::cout << "   P           : 3+4 (Flying Dropkick)\n";
    std::cout << " Player 2 (Tekken 7 4-Button Controls):\n";
    std::cout << "   Arrows      : Move / Guard / Crouch / Jump\n";
    std::cout << "   Num 1 or ,  : 1 (Left Punch - Flash Jab)\n";
    std::cout << "   Num 2 or .  : 2 (Right Punch - Straight Cross / Fwd+2: EWGF / 1,2: One-Two!)\n";
    std::cout << "   Num 4 or /  : 3 (Left Kick - Mid Kick / Down+3: Hell Sweep!)\n";
    std::cout << "   Num 5 or ;  : 4 (Right Kick - Axe Roundhouse / Up+4: Hopkick!)\n";
    std::cout << "   Num 3 or ]  : 1+2 (Power Crush Armor / Rage Art when in Rage!)\n";
    std::cout << "   Num 6 or '  : 1+3 (Command Throw - Unblockable Grab!)\n";
    std::cout << "   Num 9 or [  : 3+4 (Flying Dropkick)\n";
    std::cout << " Global:\n";
    std::cout << "   TAB         : Slow-Mo toggle\n";
    std::cout << "   B / Enter   : Rematch / Reset\n";
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
