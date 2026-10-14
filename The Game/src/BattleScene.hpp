#pragma once
#include "IScene.hpp"
#include "Fighter.hpp"
#include "CombatManager.hpp"
#include "CpuController.hpp"
#include "InputManager.hpp"
#include "BattleHUD.hpp"

#include <RagdollEngine/Physics/PhysicsWorld.hpp>
#include <RagdollEngine/Core/CameraDirector.hpp>
#include <RagdollEngine/Render/JuiceFX.hpp>

#include <memory>
#include <vector>

namespace StickminGame {

struct GameContext;

class BattleScene : public IScene {
public:
    explicit BattleScene(GameContext& ctx);
    ~BattleScene() override;

    void onEnter() override;
    void onExit() override;

    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    void restartMatch();
    void rematch();

private:
    GameContext& m_ctx;

    std::unique_ptr<RagdollEngine::PhysicsWorld> m_physicsWorld;
    std::vector<b2BodyId> m_props;

    std::unique_ptr<Fighter> m_p1;
    std::unique_ptr<Fighter> m_p2;

    std::unique_ptr<CombatManager> m_combatManager;
    CpuController m_cpuController;
    InputManager m_inputManager;
    BattleHUD m_battleHUD;
    RagdollEngine::CameraDirector m_camera;
    RagdollEngine::JuiceFX m_juiceFX;

    float m_p1Last1Time{ -10.0f };
    float m_p2Last1Time{ -10.0f };
    float m_hostClientMoveX{ 0.0f };
    float m_hostClientMoveY{ 0.0f };
    float m_lastClientInputTime{ 0.0f };

    void buildArenaGeometry();
};

} // namespace StickminGame
