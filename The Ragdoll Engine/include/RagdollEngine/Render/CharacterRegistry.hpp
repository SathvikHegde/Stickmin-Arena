#pragma once
#include "RagdollEngine/Render/CharacterDef.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

namespace RagdollEngine {

class CharacterRegistry {
public:
    static CharacterDefinition getHenry();
    static CharacterDefinition getEllie();
    static CharacterDefinition getCharles();
    static CharacterDefinition getReginald();
    static CharacterDefinition getRightHandMan();

    static const std::vector<CharacterDefinition>& getAllRosterCharacters();
    static CharacterDefinition getById(const std::string& id);
    static void registerCharacter(const CharacterDefinition& def);
};

} // namespace RagdollEngine
