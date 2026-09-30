#include "RagdollEngine/Render/CharacterRegistry.hpp"
#include <algorithm>

namespace RagdollEngine {

CharacterDefinition CharacterRegistry::getHenry() {
    CharacterDefinition def;
    def.id = "henry";
    def.displayName = "HENRY STICKMIN";
    def.title = "THE MASTER HEIST";

    def.headwear = HeadwearType::None;
    def.eyeStyle = EyeStyle::ClassicDot;
    def.eyebrowStyle = EyebrowStyle::Determined;
    def.mouthStyle = MouthStyle::Smirk;
    def.facialHair = FacialHairType::None;

    def.footwear = FootwearType::ClassicShoes;
    def.shoeColor = sf::Color(118, 70, 36);        // Classic warm brown leather
    def.shoeSoleColor = sf::Color(52, 28, 14);

    def.handwear = HandwearType::WhiteCartoon;
    def.gloveColor = sf::Color(252, 252, 255);

    def.accentColor = sf::Color(45, 145, 255);     // Azure Blue
    def.electricColor = sf::Color(120, 220, 255);
    def.moveSpeedMult = 1.0f;
    def.damageMult = 1.0f;
    return def;
}

CharacterDefinition CharacterRegistry::getEllie() {
    CharacterDefinition def;
    def.id = "ellie";
    def.displayName = "ELLIE ROSE";
    def.title = "CONVICT ALLY";

    def.headwear = HeadwearType::EllieHair;
    def.headwearColor = sf::Color(226, 38, 57);        // Fiery scarlet red
    def.headwearShadingColor = sf::Color(138, 16, 30);  // Darker contour shading

    def.eyeStyle = EyeStyle::FeminineLash;              // Feminine eyes with eyelash flick
    def.eyebrowStyle = EyebrowStyle::Arched;
    def.mouthStyle = MouthStyle::ConfidentGrin;
    def.facialHair = FacialHairType::None;

    def.footwear = FootwearType::CombatBoots;
    def.shoeColor = sf::Color(48, 42, 46);             // Dark laced combat boots
    def.shoeSoleColor = sf::Color(24, 20, 22);

    def.handwear = HandwearType::WhiteCartoon;
    def.gloveColor = sf::Color(252, 252, 255);

    def.accentColor = sf::Color(235, 45, 65);          // Crimson Red
    def.electricColor = sf::Color(255, 90, 110);
    def.moveSpeedMult = 1.05f;                         // Agile
    def.damageMult = 0.98f;
    return def;
}

CharacterDefinition CharacterRegistry::getCharles() {
    CharacterDefinition def;
    def.id = "charles";
    def.displayName = "CHARLES CALVIN";
    def.title = "GREATEST PLAN";

    def.headwear = HeadwearType::Headphones;
    def.headwearColor = sf::Color(225, 40, 40);        // Bright red pilot headset
    def.headwearAccentColor = sf::Color(38, 40, 46);   // Dark headband & boom mic

    def.eyeStyle = EyeStyle::ClassicDot;
    def.eyebrowStyle = EyebrowStyle::Determined;
    def.mouthStyle = MouthStyle::DeterminedLine;       // Friendly, confident smile
    def.facialHair = FacialHairType::None;

    def.footwear = FootwearType::ClassicShoes;
    def.shoeColor = sf::Color(118, 70, 36);
    def.shoeSoleColor = sf::Color(52, 28, 14);

    def.handwear = HandwearType::WhiteCartoon;
    def.gloveColor = sf::Color(252, 252, 255);

    def.accentColor = sf::Color(255, 195, 25);         // Helicopter Amber / Gold
    def.electricColor = sf::Color(255, 220, 60);
    def.moveSpeedMult = 1.02f;
    def.damageMult = 1.04f;
    return def;
}

CharacterDefinition CharacterRegistry::getReginald() {
    CharacterDefinition def;
    def.id = "reginald";
    def.displayName = "REGINALD COPPERBOTTOM";
    def.title = "TOPPAT LEADER";

    def.headwear = HeadwearType::DoubleTopHat;
    def.headwearColor = sf::Color(30, 32, 38);         // Dark felt top hats
    def.headwearAccentColor = sf::Color(255, 215, 0);  // Gold band & chest chain

    def.eyeStyle = EyeStyle::ClassicDot;
    def.eyebrowStyle = EyebrowStyle::Determined;
    def.mouthStyle = MouthStyle::Smirk;

    def.facialHair = FacialHairType::ReginaldMustache;  // Grand golden handlebar mustache
    def.facialHairColor = sf::Color(240, 195, 35);

    def.footwear = FootwearType::GoldenShoes;
    def.shoeColor = sf::Color(225, 185, 35);           // Polished golden dress shoes
    def.shoeSoleColor = sf::Color(45, 35, 15);

    def.handwear = HandwearType::WhiteCartoon;
    def.gloveColor = sf::Color(252, 252, 255);

    def.accentColor = sf::Color(255, 215, 0);          // Royal Gold
    def.electricColor = sf::Color(255, 215, 0);
    def.moveSpeedMult = 0.96f;
    def.damageMult = 1.08f;
    return def;
}

CharacterDefinition CharacterRegistry::getRightHandMan() {
    CharacterDefinition def;
    def.id = "rhm";
    def.displayName = "RIGHT HAND MAN";
    def.title = "CYBORG ENFORCER";

    def.headwear = HeadwearType::TopHat;
    def.headwearColor = sf::Color(28, 30, 36);
    def.headwearAccentColor = sf::Color(50, 140, 255); // Blue Toppat band

    def.eyeStyle = EyeStyle::CyborgLaserEye;            // Glowing red cybernetic optic
    def.eyebrowStyle = EyebrowStyle::Angry;
    def.mouthStyle = MouthStyle::Frown;

    def.facialHair = FacialHairType::RHMBigMustache;   // Giant bushy ginger mustache
    def.facialHairColor = sf::Color(230, 110, 30);

    def.footwear = FootwearType::CyberThrusters;
    def.shoeColor = sf::Color(55, 60, 70);             // Metallic dark boot
    def.shoeSoleColor = sf::Color(30, 35, 42);

    def.handwear = HandwearType::CyberneticArm;
    def.gloveColor = sf::Color(170, 180, 195);         // Chrome steel fist

    def.accentColor = sf::Color(245, 95, 30);          // Inferno Orange
    def.electricColor = sf::Color(255, 60, 40);
    def.moveSpeedMult = 0.95f;
    def.damageMult = 1.12f;
    return def;
}

static std::vector<CharacterDefinition> s_customRegistry;

const std::vector<CharacterDefinition>& CharacterRegistry::getAllRosterCharacters() {
    static std::vector<CharacterDefinition> roster;
    if (roster.empty()) {
        roster.push_back(getHenry());
        roster.push_back(getEllie());
        roster.push_back(getCharles());
        roster.push_back(getReginald());
        roster.push_back(getRightHandMan());
        for (const auto& custom : s_customRegistry) {
            roster.push_back(custom);
        }
    }
    return roster;
}

CharacterDefinition CharacterRegistry::getById(const std::string& id) {
    if (id == "henry") return getHenry();
    if (id == "ellie") return getEllie();
    if (id == "charles") return getCharles();
    if (id == "reginald") return getReginald();
    if (id == "rhm") return getRightHandMan();

    for (const auto& c : s_customRegistry) {
        if (c.id == id) return c;
    }
    return getHenry(); // Default fallback
}

void CharacterRegistry::registerCharacter(const CharacterDefinition& def) {
    s_customRegistry.push_back(def);
}

} // namespace RagdollEngine
