#pragma once
#include <RagdollEngine/Render/CharacterDef.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace StickminGame {

class CharacterRegistry {
public:
    static RagdollEngine::CharacterDefinition getHenry();
    static RagdollEngine::CharacterDefinition getEllie();
    static RagdollEngine::CharacterDefinition getCharles();
    static RagdollEngine::CharacterDefinition getReginald();
    static RagdollEngine::CharacterDefinition getRightHandMan();

    static const std::vector<RagdollEngine::CharacterDefinition>& getAllRosterCharacters();
    static RagdollEngine::CharacterDefinition getById(const std::string& id);
    static void registerCharacter(const RagdollEngine::CharacterDefinition& def);
};

} // namespace StickminGame

// Backward-compatibility alias for smooth transition
namespace RagdollEngine {
    using CharacterRegistry = StickminGame::CharacterRegistry;
}
