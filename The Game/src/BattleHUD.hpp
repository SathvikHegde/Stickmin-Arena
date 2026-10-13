#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "Fighter.hpp"
#include "CombatManager.hpp"
#include "CpuController.hpp"
#include "MenuManager.hpp"

#include <RagdollEngine/Render/StageRenderer.hpp>

namespace StickminGame {

class NetworkManager;

enum class MatchOverAction {
    None,
    Rematch,
    ChooseFighters,
    ChangeStage,
    MainMenu
};

class BattleHUD {
public:
    explicit BattleHUD(const sf::Font* font = nullptr);

    void setFont(const sf::Font* font) { m_font = font; }
    const sf::Font* getFont() const { return m_font; }

    void draw(sf::RenderWindow& window,
              const Fighter& p1,
              const Fighter& p2,
              const CombatManager& combatManager,
              GameMode gameMode,
              float gameTime,
              const RagdollEngine::StageRenderer& stageRenderer,
              CpuDifficulty cpuDifficulty = CpuDifficulty::Medium,
              const NetworkManager* netManager = nullptr);

    MatchOverAction checkMatchOverClick(const sf::Vector2f& uiMousePos) const;

private:
    const sf::Font* m_font{ nullptr };

    void drawHealthBarP1(sf::RenderWindow& window, const Fighter& p1, float topY, float barWidth, float barHeight, float ragePulse);
    void drawHealthBarP2(sf::RenderWindow& window, const Fighter& p2, float topY, float barWidth, float barHeight, float ragePulse, GameMode gameMode, CpuDifficulty cpuDifficulty);
    void drawTimerAndRounds(sf::RenderWindow& window, const CombatManager& combatManager, GameMode gameMode, float topY, float barHeight);
    void drawCombosAndAlerts(sf::RenderWindow& window, const Fighter& p1, const Fighter& p2);
    void drawBanners(sf::RenderWindow& window, const Fighter& p1, const Fighter& p2, const CombatManager& combatManager, const RagdollEngine::StageRenderer& stageRenderer, const sf::Vector2f& mouseScreenPos);
    void drawNetplayPill(sf::RenderWindow& window, const NetworkManager& netManager);
    void drawQuickHelp(sf::RenderWindow& window, GameMode gameMode, CpuDifficulty cpuDifficulty, const NetworkManager* netManager);
};

} // namespace StickminGame
