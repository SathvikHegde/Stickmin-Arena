#include "BattleHUD.hpp"
#include "NetworkManager.hpp"

#include <cmath>
#include <algorithm>
#include <string>

namespace StickminGame {

BattleHUD::BattleHUD(const sf::Font* font)
    : m_font(font) {
}

void BattleHUD::draw(sf::RenderWindow& window,
                     const Fighter& p1,
                     const Fighter& p2,
                     const CombatManager& combatManager,
                     GameMode gameMode,
                     float gameTime,
                     const RagdollEngine::StageRenderer& stageRenderer,
                     CpuDifficulty cpuDifficulty,
                     const NetworkManager* netManager) {
    if (!m_font) return;

    float topY = 32.0f;
    float barWidth = 560.0f;
    float barHeight = 26.0f;
    float ragePulse = (std::sin(gameTime * 9.0f) + 1.0f) * 0.5f;

    // 1. Player Health Bars & Victory Gems
    drawHealthBarP1(window, p1, topY, barWidth, barHeight, ragePulse);
    drawHealthBarP2(window, p2, topY, barWidth, barHeight, ragePulse, gameMode, cpuDifficulty);

    // 2. Timer & Round Banners
    drawTimerAndRounds(window, combatManager, gameMode, topY, barHeight);

    // 3. Combos & Floating Combat Alerts
    drawCombosAndAlerts(window, p1, p2);

    // 4. Online Netplay Pill (if in Online mode)
    if (gameMode == GameMode::Online && netManager) {
        drawNetplayPill(window, *netManager);
    }

    // 5. Match State Banners (Round intro, KO, Winner + Rematch buttons)
    sf::Vector2i mPix = sf::Mouse::getPosition(window);
    sf::Vector2f mouseScreenPos = window.mapPixelToCoords(mPix, window.getView());
    drawBanners(window, p1, p2, combatManager, stageRenderer, mouseScreenPos);

    // 6. Quick Move Reference at the Bottom
    drawQuickHelp(window, gameMode, cpuDifficulty, netManager);
}

void BattleHUD::drawHealthBarP1(sf::RenderWindow& window, const Fighter& p1, float topY, float barWidth, float barHeight, float ragePulse) {
    // --- P1 HEALTH BAR (Top Left) ---
    // Background slot with Tekken 7 Rage pulse
    sf::RectangleShape p1Bg(sf::Vector2f(barWidth, barHeight));
    p1Bg.setPosition(sf::Vector2f(90.0f, topY));
    p1Bg.setFillColor(sf::Color(25, 28, 36, 230));
    if (p1.isInRage()) {
        p1Bg.setOutlineColor(sf::Color(255, 30 + static_cast<std::uint8_t>(50 * ragePulse), 30));
        p1Bg.setOutlineThickness(3.5f);
    } else {
        p1Bg.setOutlineColor(sf::Color(80, 90, 110));
        p1Bg.setOutlineThickness(2.0f);
    }
    window.draw(p1Bg);

    // Ghost Lag Bar (Yellow damage lag)
    float p1GhostWidth = (p1.getGhostHealth() / p1.getMaxHealth()) * barWidth;
    if (p1GhostWidth > 0.0f) {
        sf::RectangleShape p1Ghost(sf::Vector2f(p1GhostWidth, barHeight));
        p1Ghost.setPosition(sf::Vector2f(90.0f + (barWidth - p1GhostWidth), topY)); // Drains toward center
        p1Ghost.setFillColor(sf::Color(255, 200, 50, 220));
        window.draw(p1Ghost);
    }

    // Current Health Bar (Dynamic character accent color)
    float p1CurWidth = (p1.getHealth() / p1.getMaxHealth()) * barWidth;
    if (p1CurWidth > 0.0f) {
        sf::RectangleShape p1Fill(sf::Vector2f(p1CurWidth, barHeight));
        p1Fill.setPosition(sf::Vector2f(90.0f + (barWidth - p1CurWidth), topY));
        p1Fill.setFillColor(p1.getCharacterDef().accentColor);
        window.draw(p1Fill);
    }

    // P1 Nameplate & Title
    sf::Text p1Name(*m_font, p1.getName(), 18);
    p1Name.setStyle(sf::Text::Bold);
    p1Name.setFillColor(p1.getCharacterDef().accentColor);
    p1Name.setPosition(sf::Vector2f(90.0f, topY - 26.0f));
    window.draw(p1Name);

    // P1 RAGE Badge
    if (p1.isInRage()) {
        sf::RectangleShape p1RageBadge(sf::Vector2f(56.0f, 18.0f));
        p1RageBadge.setPosition(sf::Vector2f(90.0f + p1Name.getLocalBounds().size.x + 12.0f, topY - 24.0f));
        p1RageBadge.setFillColor(sf::Color(190, 25, 25, 240));
        p1RageBadge.setOutlineColor(sf::Color(255, 225, 40));
        p1RageBadge.setOutlineThickness(1.5f);
        window.draw(p1RageBadge);

        sf::Text p1RageText(*m_font, "RAGE", 11);
        p1RageText.setStyle(sf::Text::Bold);
        p1RageText.setFillColor(sf::Color(255, 245, 100));
        p1RageText.setPosition(sf::Vector2f(90.0f + p1Name.getLocalBounds().size.x + 23.0f, topY - 23.0f));
        window.draw(p1RageText);
    }

    // P1 Victory Gems
    for (int r = 0; r < 2; ++r) {
        sf::CircleShape gem(6.0f);
        gem.setOrigin(sf::Vector2f(6.0f, 6.0f));
        gem.setPosition(sf::Vector2f(620.0f - r * 20.0f, topY + barHeight + 14.0f));
        gem.setFillColor(r < p1.getRoundsWon() ? sf::Color(255, 215, 0) : sf::Color(40, 45, 55));
        gem.setOutlineColor(sf::Color(255, 255, 255, 160));
        gem.setOutlineThickness(1.5f);
        window.draw(gem);
    }
}

void BattleHUD::drawHealthBarP2(sf::RenderWindow& window, const Fighter& p2, float topY, float barWidth, float barHeight, float ragePulse, GameMode gameMode, CpuDifficulty cpuDifficulty) {
    // --- P2 HEALTH BAR (Top Right) ---
    // Background slot with Tekken 7 Rage pulse
    sf::RectangleShape p2Bg(sf::Vector2f(barWidth, barHeight));
    p2Bg.setPosition(sf::Vector2f(950.0f, topY));
    p2Bg.setFillColor(sf::Color(25, 28, 36, 230));
    if (p2.isInRage()) {
        p2Bg.setOutlineColor(sf::Color(255, 30 + static_cast<std::uint8_t>(50 * ragePulse), 30));
        p2Bg.setOutlineThickness(3.5f);
    } else {
        p2Bg.setOutlineColor(sf::Color(80, 90, 110));
        p2Bg.setOutlineThickness(2.0f);
    }
    window.draw(p2Bg);

    // Ghost Lag Bar
    float p2GhostWidth = (p2.getGhostHealth() / p2.getMaxHealth()) * barWidth;
    if (p2GhostWidth > 0.0f) {
        sf::RectangleShape p2Ghost(sf::Vector2f(p2GhostWidth, barHeight));
        p2Ghost.setPosition(sf::Vector2f(950.0f, topY)); // Drains toward center
        p2Ghost.setFillColor(sf::Color(255, 200, 50, 220));
        window.draw(p2Ghost);
    }

    // Current Health Bar (Dynamic character accent color)
    float p2CurWidth = (p2.getHealth() / p2.getMaxHealth()) * barWidth;
    if (p2CurWidth > 0.0f) {
        sf::RectangleShape p2Fill(sf::Vector2f(p2CurWidth, barHeight));
        p2Fill.setPosition(sf::Vector2f(950.0f, topY));
        p2Fill.setFillColor(p2.getCharacterDef().accentColor);
        window.draw(p2Fill);
    }

    // P2 Nameplate & Title
    std::string p2DisplayName = p2.getName();
    if (gameMode == GameMode::VsCpu) {
        p2DisplayName += " [CPU: " + std::string(getDifficultyName(cpuDifficulty)) + "]";
    }
    sf::Text p2Name(*m_font, p2DisplayName, 18);
    p2Name.setStyle(sf::Text::Bold);
    p2Name.setFillColor(p2.getCharacterDef().accentColor);
    p2Name.setOrigin(sf::Vector2f(p2Name.getLocalBounds().size.x, 0.0f));
    p2Name.setPosition(sf::Vector2f(1510.0f, topY - 26.0f));
    window.draw(p2Name);

    // P2 RAGE Badge
    if (p2.isInRage()) {
        sf::RectangleShape p2RageBadge(sf::Vector2f(56.0f, 18.0f));
        p2RageBadge.setPosition(sf::Vector2f(1510.0f - p2Name.getLocalBounds().size.x - 68.0f, topY - 24.0f));
        p2RageBadge.setFillColor(sf::Color(190, 25, 25, 240));
        p2RageBadge.setOutlineColor(sf::Color(255, 225, 40));
        p2RageBadge.setOutlineThickness(1.5f);
        window.draw(p2RageBadge);

        sf::Text p2RageText(*m_font, "RAGE", 11);
        p2RageText.setStyle(sf::Text::Bold);
        p2RageText.setFillColor(sf::Color(255, 245, 100));
        p2RageText.setPosition(sf::Vector2f(1510.0f - p2Name.getLocalBounds().size.x - 57.0f, topY - 23.0f));
        window.draw(p2RageText);
    }

    // P2 Victory Gems
    for (int r = 0; r < 2; ++r) {
        sf::CircleShape gem(6.0f);
        gem.setOrigin(sf::Vector2f(6.0f, 6.0f));
        gem.setPosition(sf::Vector2f(980.0f + r * 20.0f, topY + barHeight + 14.0f));
        gem.setFillColor(r < p2.getRoundsWon() ? sf::Color(255, 215, 0) : sf::Color(40, 45, 55));
        gem.setOutlineColor(sf::Color(255, 255, 255, 160));
        gem.setOutlineThickness(1.5f);
        window.draw(gem);
    }
}

void BattleHUD::drawTimerAndRounds(sf::RenderWindow& window, const CombatManager& combatManager, GameMode gameMode, float topY, float barHeight) {
    // --- ROUND TIMER (Top Center) ---
    sf::RectangleShape timerFrame(sf::Vector2f(110.0f, 64.0f));
    timerFrame.setOrigin(sf::Vector2f(55.0f, 32.0f));
    timerFrame.setPosition(sf::Vector2f(800.0f, topY + barHeight * 0.5f));
    timerFrame.setFillColor(sf::Color(16, 18, 24, 240));
    timerFrame.setOutlineColor(sf::Color(180, 190, 215));
    timerFrame.setOutlineThickness(2.5f);
    window.draw(timerFrame);

    int sec = static_cast<int>(std::ceil(combatManager.getRoundTimer()));
    std::string timerStr = (gameMode == GameMode::Practice) ? "--" : std::to_string(sec);
    sf::Text timerText(*m_font, timerStr, 32);
    timerText.setStyle(sf::Text::Bold);
    timerText.setFillColor(sec <= 10 && gameMode != GameMode::Practice ? sf::Color(255, 60, 60) : sf::Color(245, 245, 250));
    timerText.setOrigin(sf::Vector2f(timerText.getLocalBounds().size.x * 0.5f, timerText.getLocalBounds().size.y * 0.5f + 4.0f));
    timerText.setPosition(sf::Vector2f(800.0f, topY + barHeight * 0.5f));
    window.draw(timerText);

    // Round Number Banner
    std::string roundStr = (gameMode == GameMode::Practice)
                         ? "PRACTICE"
                         : (gameMode == GameMode::VsCpu)
                         ? ("VS CPU // ROUND " + std::to_string(combatManager.getRoundNumber()))
                         : ("ROUND " + std::to_string(combatManager.getRoundNumber()));
    sf::Text roundSub(*m_font, roundStr, 11);
    roundSub.setStyle(sf::Text::Bold);
    roundSub.setFillColor(sf::Color(160, 175, 200));
    roundSub.setOrigin(sf::Vector2f(roundSub.getLocalBounds().size.x * 0.5f, 0.0f));
    roundSub.setPosition(sf::Vector2f(800.0f, topY + barHeight + 12.0f));
    window.draw(roundSub);
}

void BattleHUD::drawCombosAndAlerts(sf::RenderWindow& window, const Fighter& p1, const Fighter& p2) {
    // --- P1 DYNAMIC COMBO DISPLAY ---
    if (p1.getComboHits() > 1) {
        sf::Text p1ComboHits(*m_font, std::to_string(p1.getComboHits()) + " HITS", 38);
        p1ComboHits.setStyle(sf::Text::Bold | sf::Text::Italic);
        p1ComboHits.setFillColor(sf::Color(60, 160, 255));
        p1ComboHits.setOutlineColor(sf::Color::Black);
        p1ComboHits.setOutlineThickness(3.0f);
        p1ComboHits.setPosition(sf::Vector2f(90.0f, 150.0f));
        window.draw(p1ComboHits);

        sf::Text p1ComboDmg(*m_font, "DAMAGE " + std::to_string(static_cast<int>(p1.getComboDamage())), 18);
        p1ComboDmg.setStyle(sf::Text::Bold | sf::Text::Italic);
        p1ComboDmg.setFillColor(sf::Color(255, 220, 60));
        p1ComboDmg.setPosition(sf::Vector2f(92.0f, 200.0f));
        window.draw(p1ComboDmg);
    }

    // --- P1 TEKKEN 7 COMBAT ALERT (Technical Cue) ---
    const auto& p1Alert = p1.getAlert();
    if (p1Alert.timer > 0.0f && !p1Alert.text.empty()) {
        float alphaFactor = std::clamp(p1Alert.timer / 0.32f, 0.0f, 1.0f);
        std::uint8_t textAlpha = static_cast<std::uint8_t>(255 * alphaFactor);
        float alertY = (p1.getComboHits() > 1) ? 234.0f : 148.0f;

        sf::Text alertText(*m_font, p1Alert.text, 20);
        alertText.setStyle(sf::Text::Bold | sf::Text::Italic);
        alertText.setFillColor(sf::Color(p1Alert.color.r, p1Alert.color.g, p1Alert.color.b, textAlpha));
        alertText.setOutlineColor(sf::Color(0, 0, 0, textAlpha));
        alertText.setOutlineThickness(2.5f);

        sf::FloatRect bounds = alertText.getLocalBounds();
        sf::RectangleShape badge(sf::Vector2f(bounds.size.x + 22.0f, 28.0f));
        badge.setPosition(sf::Vector2f(88.0f, alertY - 2.0f));
        badge.setFillColor(sf::Color(14, 18, 26, static_cast<std::uint8_t>(210 * alphaFactor)));
        badge.setOutlineColor(sf::Color(p1Alert.color.r, p1Alert.color.g, p1Alert.color.b, static_cast<std::uint8_t>(180 * alphaFactor)));
        badge.setOutlineThickness(1.5f);
        window.draw(badge);

        alertText.setPosition(sf::Vector2f(98.0f, alertY));
        window.draw(alertText);
    }

    // --- P2 DYNAMIC COMBO DISPLAY ---
    if (p2.getComboHits() > 1) {
        sf::Text p2ComboHits(*m_font, std::to_string(p2.getComboHits()) + " HITS", 38);
        p2ComboHits.setStyle(sf::Text::Bold | sf::Text::Italic);
        p2ComboHits.setFillColor(sf::Color(255, 60, 80));
        p2ComboHits.setOutlineColor(sf::Color::Black);
        p2ComboHits.setOutlineThickness(3.0f);
        p2ComboHits.setOrigin(sf::Vector2f(p2ComboHits.getLocalBounds().size.x, 0.0f));
        p2ComboHits.setPosition(sf::Vector2f(1510.0f, 150.0f));
        window.draw(p2ComboHits);

        sf::Text p2ComboDmg(*m_font, "DAMAGE " + std::to_string(static_cast<int>(p2.getComboDamage())), 18);
        p2ComboDmg.setStyle(sf::Text::Bold | sf::Text::Italic);
        p2ComboDmg.setFillColor(sf::Color(255, 220, 60));
        p2ComboDmg.setOrigin(sf::Vector2f(p2ComboDmg.getLocalBounds().size.x, 0.0f));
        p2ComboDmg.setPosition(sf::Vector2f(1508.0f, 200.0f));
        window.draw(p2ComboDmg);
    }

    // --- P2 TEKKEN 7 COMBAT ALERT (Technical Cue) ---
    const auto& p2Alert = p2.getAlert();
    if (p2Alert.timer > 0.0f && !p2Alert.text.empty()) {
        float alphaFactor = std::clamp(p2Alert.timer / 0.32f, 0.0f, 1.0f);
        std::uint8_t textAlpha = static_cast<std::uint8_t>(255 * alphaFactor);
        float alertY = (p2.getComboHits() > 1) ? 234.0f : 148.0f;

        sf::Text alertText(*m_font, p2Alert.text, 20);
        alertText.setStyle(sf::Text::Bold | sf::Text::Italic);
        alertText.setFillColor(sf::Color(p2Alert.color.r, p2Alert.color.g, p2Alert.color.b, textAlpha));
        alertText.setOutlineColor(sf::Color(0, 0, 0, textAlpha));
        alertText.setOutlineThickness(2.5f);

        sf::FloatRect bounds = alertText.getLocalBounds();
        sf::RectangleShape badge(sf::Vector2f(bounds.size.x + 22.0f, 28.0f));
        badge.setOrigin(sf::Vector2f(badge.getSize().x, 0.0f));
        badge.setPosition(sf::Vector2f(1512.0f, alertY - 2.0f));
        badge.setFillColor(sf::Color(14, 18, 26, static_cast<std::uint8_t>(210 * alphaFactor)));
        badge.setOutlineColor(sf::Color(p2Alert.color.r, p2Alert.color.g, p2Alert.color.b, static_cast<std::uint8_t>(180 * alphaFactor)));
        badge.setOutlineThickness(1.5f);
        window.draw(badge);

        alertText.setOrigin(sf::Vector2f(bounds.size.x, 0.0f));
        alertText.setPosition(sf::Vector2f(1502.0f, alertY));
        window.draw(alertText);
    }
}

void BattleHUD::drawNetplayPill(sf::RenderWindow& window, const NetworkManager& netManager) {
    float ping = netManager.getPingMs();
    sf::Color pingCol = (ping < 50.0f) ? sf::Color(80, 240, 140) : (ping < 110.0f) ? sf::Color(255, 205, 50) : sf::Color(255, 75, 75);
    std::string roleStr = netManager.isHost() ? "ONLINE [HOST]" : "ONLINE [CLIENT]";
    std::string pingStr = roleStr + "  |  " + std::to_string(static_cast<int>(ping)) + " ms";

    float pillX = 24.0f;
    float pillY = 860.0f;
    float pillW = 190.0f;
    float pillH = 24.0f;

    sf::RectangleShape netPill(sf::Vector2f(pillW, pillH));
    netPill.setPosition(sf::Vector2f(pillX, pillY));
    netPill.setFillColor(sf::Color(14, 18, 26, 225));
    netPill.setOutlineColor(pingCol);
    netPill.setOutlineThickness(1.5f);
    window.draw(netPill);

    sf::CircleShape pingDot(4.0f);
    pingDot.setOrigin(sf::Vector2f(4.0f, 4.0f));
    pingDot.setPosition(sf::Vector2f(pillX + 13.0f, pillY + pillH * 0.5f));
    pingDot.setFillColor(pingCol);
    window.draw(pingDot);

    sf::Text netText(*m_font, pingStr, 11);
    netText.setStyle(sf::Text::Bold);
    netText.setFillColor(sf::Color(220, 235, 250));
    netText.setPosition(sf::Vector2f(pillX + 24.0f, pillY + 4.0f));
    window.draw(netText);
}

void BattleHUD::drawBanners(sf::RenderWindow& window, const Fighter& p1, const Fighter& p2, const CombatManager& combatManager, const RagdollEngine::StageRenderer& stageRenderer, const sf::Vector2f& mouseScreenPos) {
    if (combatManager.getState() == MatchState::RoundIntro) {
        std::string announceStr = (combatManager.getStateTimer() > 0.65f)
            ? ("ROUND " + std::to_string(combatManager.getRoundNumber()))
            : "FIGHT";
        sf::Color announceCol = (combatManager.getStateTimer() > 0.65f)
            ? sf::Color(255, 230, 70)
            : sf::Color(255, 80, 50);

        // Central cinematic banner plate
        sf::RectangleShape bannerPlate(sf::Vector2f(1600.0f, 96.0f));
        bannerPlate.setOrigin(sf::Vector2f(800.0f, 48.0f));
        bannerPlate.setPosition(sf::Vector2f(800.0f, 365.0f));
        bannerPlate.setFillColor(sf::Color(10, 12, 18, 205));
        bannerPlate.setOutlineColor(announceCol);
        bannerPlate.setOutlineThickness(2.0f);
        window.draw(bannerPlate);

        sf::Text introText(*m_font, announceStr, 60);
        introText.setStyle(sf::Text::Bold | sf::Text::Italic);
        introText.setFillColor(announceCol);
        introText.setOutlineColor(sf::Color(10, 10, 15));
        introText.setOutlineThickness(4.5f);
        introText.setOrigin(sf::Vector2f(introText.getLocalBounds().size.x * 0.5f, introText.getLocalBounds().size.y * 0.5f));
        introText.setPosition(sf::Vector2f(800.0f, 360.0f));
        window.draw(introText);

        // Stage Presentation Banner
        sf::Text stageBanner(*m_font, stageRenderer.getStageName() + "  //  " + stageRenderer.getStageSubtitle(), 15);
        stageBanner.setStyle(sf::Text::Bold);
        stageBanner.setFillColor(stageRenderer.getStageThemeColor());
        stageBanner.setOutlineColor(sf::Color(10, 10, 15));
        stageBanner.setOutlineThickness(2.5f);
        stageBanner.setOrigin(sf::Vector2f(stageBanner.getLocalBounds().size.x * 0.5f, 0.0f));
        stageBanner.setPosition(sf::Vector2f(800.0f, 428.0f));
        window.draw(stageBanner);
    } else if (combatManager.getState() == MatchState::RoundKO) {
        std::string koStr = "K.O.";
        if (p1.isDead() && p2.isDead()) {
            koStr = "DOUBLE K.O.";
        } else if (combatManager.getRoundTimer() <= 0.0f && !p1.isDead() && !p2.isDead()) {
            koStr = "TIME UP";
        }

        // Tekken 7 Cinematic K.O. Bar
        sf::RectangleShape koPlate(sf::Vector2f(1600.0f, 120.0f));
        koPlate.setOrigin(sf::Vector2f(800.0f, 60.0f));
        koPlate.setPosition(sf::Vector2f(800.0f, 365.0f));
        koPlate.setFillColor(sf::Color(12, 14, 20, 220));
        koPlate.setOutlineColor(sf::Color(255, 200, 40, 225));
        koPlate.setOutlineThickness(2.5f);
        window.draw(koPlate);

        sf::Text koText(*m_font, koStr, 84);
        koText.setStyle(sf::Text::Bold | sf::Text::Italic);
        koText.setFillColor(sf::Color(255, 225, 50));
        koText.setOutlineColor(sf::Color(15, 15, 20));
        koText.setOutlineThickness(5.0f);
        koText.setOrigin(sf::Vector2f(koText.getLocalBounds().size.x * 0.5f, koText.getLocalBounds().size.y * 0.5f));
        koText.setPosition(sf::Vector2f(800.0f, 355.0f));
        window.draw(koText);

        // Perfect Check
        int winner = combatManager.getRoundWinner();
        bool isPerfect = (winner == 1 && p1.getHealth() >= 100.0f) ||
                         (winner == 2 && p2.getHealth() >= 100.0f);
        if (isPerfect) {
            sf::Text perfText(*m_font, "PERFECT", 30);
            perfText.setStyle(sf::Text::Bold | sf::Text::Italic);
            perfText.setFillColor(sf::Color(90, 230, 255));
            perfText.setOutlineColor(sf::Color(10, 10, 15));
            perfText.setOutlineThickness(3.0f);
            perfText.setOrigin(sf::Vector2f(perfText.getLocalBounds().size.x * 0.5f, 0.0f));
            perfText.setPosition(sf::Vector2f(800.0f, 435.0f));
            window.draw(perfText);
        }
    } else if (combatManager.getState() == MatchState::MatchOver) {
        std::string winnerStr = (combatManager.getRoundWinner() == 1) ? (p1.getName() + " WINS") : (p2.getName() + " WINS");
        sf::Text winText(*m_font, winnerStr, 54);
        winText.setStyle(sf::Text::Bold | sf::Text::Italic);
        winText.setFillColor(combatManager.getRoundWinner() == 1 ? p1.getCharacterDef().accentColor : p2.getCharacterDef().accentColor);
        winText.setOutlineColor(sf::Color(10, 10, 15));
        winText.setOutlineThickness(4.0f);
        winText.setOrigin(sf::Vector2f(winText.getLocalBounds().size.x * 0.5f, winText.getLocalBounds().size.y * 0.5f));
        winText.setPosition(sf::Vector2f(800.0f, 370.0f));
        window.draw(winText);

        // 4 Interactive Action Buttons
        auto drawEndBtn = [&](float x, float y, float w, float h, const std::string& label, sf::Color col) {
            sf::FloatRect rect(sf::Vector2f(x, y), sf::Vector2f(w, h));
            bool hovered = rect.contains(mouseScreenPos);
            sf::RectangleShape b(sf::Vector2f(w, h));
            b.setPosition(sf::Vector2f(x, y));
            b.setFillColor(hovered ? sf::Color(32, 44, 62, 245) : sf::Color(18, 22, 32, 235));
            b.setOutlineColor(hovered ? sf::Color::White : col);
            b.setOutlineThickness(hovered ? 3.0f : 2.0f);
            window.draw(b);

            sf::Text t(*m_font, label, 12);
            t.setStyle(sf::Text::Bold);
            t.setFillColor(hovered ? sf::Color(255, 235, 100) : sf::Color::White);
            t.setOrigin(sf::Vector2f(t.getLocalBounds().size.x * 0.5f, t.getLocalBounds().size.y * 0.5f + 3.0f));
            t.setPosition(sf::Vector2f(x + w * 0.5f, y + h * 0.5f));
            window.draw(t);
        };

        drawEndBtn(260.0f, 475.0f, 220.0f, 44.0f, "REMATCH [ENTER]", sf::Color(255, 215, 60));
        drawEndBtn(520.0f, 475.0f, 250.0f, 44.0f, "CHOOSE FIGHTERS [C]", sf::Color(45, 145, 255));
        drawEndBtn(810.0f, 475.0f, 220.0f, 44.0f, "CHANGE STAGE [S]", sf::Color(245, 55, 65));
        drawEndBtn(1070.0f, 475.0f, 220.0f, 44.0f, "MAIN MENU [ESC]", sf::Color(180, 195, 215));
    }
}

void BattleHUD::drawQuickHelp(sf::RenderWindow& window, GameMode gameMode, CpuDifficulty cpuDifficulty, const NetworkManager* netManager) {
    std::string helpStr;
    if (gameMode == GameMode::Online && netManager) {
        helpStr = std::string(netManager->isHost() ? "[P1 HOST - YOU] " : "[P2 CLIENT - YOU] ") +
                  "1 (J), 2 (K), 3 (U), 4 (I) | 1,2: (J->K) | 1+2: Power Crush / Rage Art (O) | 1+3: Throw (L) | 3+4: Dropkick (P)\n"
                  "Fwd+2: EWGF Launcher | Down+3: Hell Sweep | Up+4: Hopkick | Block: Hold Back | Crouch Block: Down+Back  |  [ESC] Pause";
    } else if (gameMode == GameMode::VsCpu) {
        helpStr = "[VS CPU // " + std::string(getDifficultyName(cpuDifficulty)) + "]  |  [ESC] Pause  |  [F3] Move List  |  [TAB] Slow-Mo\n"
                  "P1: 1 (J), 2 (K), 3 (U), 4 (I) | 1,2: (J->K) | 1+2: Power Crush / Rage Art (O) | 1+3: Throw (L) | 3+4: Dropkick (P)\n"
                  "Fwd+2: EWGF Launcher | Down+3: Hell Sweep | Up+4: Hopkick | Block: Hold Back | Crouch Block: Down+Back";
    } else if (gameMode == GameMode::Practice) {
        helpStr = "[R] Reset Fighters  |  [F3] Move List  |  [ESC] Pause Menu\n"
                  "P1: 1 (J), 2 (K), 3 (U), 4 (I) | 1,2: (J->K) | 1+2: Power Crush (O) | 1+3: Throw (L) | 3+4: Dropkick (P)\n"
                  "Fwd+2: EWGF Launcher | Down+3: Hell Sweep | Up+4: Hopkick | Block: Hold Back | Crouch Block: Down+Back";
    } else {
        helpStr = "[ESC] Pause  |  [F3] Move List  |  [TAB] Slow-Mo\n"
                  "P1: 1 (J), 2 (K), 3 (U), 4 (I) | 1,2: (J->K) | 1+2: Power Crush / Rage Art (O) | 1+3: Throw (L) | 3+4: Dropkick (P)\n"
                  "P2: 1 (Num1), 2 (Num2), 3 (Num4), 4 (Num5) | 1+2: (Num3) | 1+3: Throw (Num6) | 3+4: (Num9)\n"
                  "Fwd+2: EWGF Launcher | Down+3: Hell Sweep | Up+4: Hopkick | Block: Hold Back | Crouch Block: Down+Back";
    }

    sf::Text moveHelp(*m_font, helpStr, 12);
    moveHelp.setFillColor(sf::Color(150, 165, 185));
    moveHelp.setOrigin(sf::Vector2f(moveHelp.getLocalBounds().size.x * 0.5f, 0.0f));
    moveHelp.setPosition(sf::Vector2f(800.0f, 835.0f));
    window.draw(moveHelp);
}

MatchOverAction BattleHUD::checkMatchOverClick(const sf::Vector2f& uiMousePos) const {
    if (sf::FloatRect(sf::Vector2f(260.0f, 475.0f), sf::Vector2f(220.0f, 44.0f)).contains(uiMousePos)) {
        return MatchOverAction::Rematch;
    }
    if (sf::FloatRect(sf::Vector2f(520.0f, 475.0f), sf::Vector2f(250.0f, 44.0f)).contains(uiMousePos)) {
        return MatchOverAction::ChooseFighters;
    }
    if (sf::FloatRect(sf::Vector2f(810.0f, 475.0f), sf::Vector2f(220.0f, 44.0f)).contains(uiMousePos)) {
        return MatchOverAction::ChangeStage;
    }
    if (sf::FloatRect(sf::Vector2f(1070.0f, 475.0f), sf::Vector2f(220.0f, 44.0f)).contains(uiMousePos)) {
        return MatchOverAction::MainMenu;
    }
    return MatchOverAction::None;
}

} // namespace StickminGame
