#pragma once
#include <SFML/Graphics/Color.hpp>
#include <string>

namespace RagdollEngine {

// 1. Pluggable Headwear / Hair
enum class HeadwearType {
    None,            // Bald (Henry Stickmin)
    EllieHair,       // Ellie Rose (Scarlet cascading spiky hair & front bangs)
    Headphones,      // Charles Calvin (Red pilot headset + boom mic)
    DoubleTopHat,    // Reginald Copperbottom (Stacked double top hats!)
    TopHat,          // Toppat Clan (Black felt top hat with band)
    MilitaryCap,     // General Galeforce
    Ushanka          // Dmitri Johannes Petrov
};

// 2. Facial Features
enum class EyeStyle {
    ClassicDot,      // Henry Stickmin (Smooth vertical oval pills + specular shine)
    FeminineLash,    // Ellie Rose (Smooth oval eyes with eyelash flick at outer corner)
    Deadpan,         // Burt Curtis (Half-lidded cynical eyes)
    AngryGlare,      // Right Hand Man / Dmitri (Intense angled glare)
    CyborgLaserEye   // Right Hand Man Reborn (Glowing cybernetic optic)
};

enum class EyebrowStyle {
    Determined,      // Angled down toward opponent (Henry)
    Arched,          // Sleek arched feminine (Ellie)
    Angry,           // Hard diagonal (RHM / Dmitri)
    Bushy            // Thick bushy (Galeforce)
};

enum class MouthStyle {
    Smirk,           // Henry's cocky smirk
    ConfidentGrin,   // Ellie's confident grin
    DeterminedLine,  // Charles' cheerful smile
    Frown            // Serious / angry
};

enum class FacialHairType {
    None,
    ReginaldMustache,// Luxurious golden handlebar mustache
    RHMBigMustache,  // Bushy orange handlebar mustache
    GaleforceWhite   // General's white mustache
};

// 3. Footwear & Handwear
enum class FootwearType {
    ClassicShoes,    // Rounded brown leather bean shoes with soles (Henry, Charles)
    CombatBoots,     // Dark laced combat boots with raised ankle collars (Ellie)
    GoldenShoes,     // Gold dress shoes (Reginald)
    CyberThrusters   // Rocket thrusters with exhaust ports (RHM)
};

enum class HandwearType {
    WhiteCartoon,    // Standard cartoon white gloves (Henry, Ellie, Charles)
    BlackLeather,    // Dark leather gloves (Toppat elites)
    CyberneticArm    // Robotic chrome arm (RHM Reborn)
};

// 4. Complete Unified Character Definition
struct CharacterDefinition {
    std::string id{ "henry" };
    std::string displayName{ "HENRY STICKMIN" };
    std::string title{ "THE MASTER HEIST" };

    // Body styling
    sf::Color bodyColor{ sf::Color(20, 20, 25) };
    sf::Color headFillColor{ sf::Color(252, 252, 255) };
    sf::Color headOutlineColor{ sf::Color(18, 18, 22) };

    // Headwear & Hair
    HeadwearType headwear{ HeadwearType::None };
    sf::Color headwearColor{ sf::Color(226, 38, 57) };        // Primary color (hair or hat felt)
    sf::Color headwearAccentColor{ sf::Color(255, 215, 0) };  // Band / trim color (gold band, etc.)
    sf::Color headwearShadingColor{ sf::Color(138, 16, 30) }; // Hair shading / shadow

    // Facial features
    EyeStyle eyeStyle{ EyeStyle::ClassicDot };
    EyebrowStyle eyebrowStyle{ EyebrowStyle::Determined };
    MouthStyle mouthStyle{ MouthStyle::Smirk };
    FacialHairType facialHair{ FacialHairType::None };
    sf::Color facialHairColor{ sf::Color(230, 180, 40) };

    // Footwear
    FootwearType footwear{ FootwearType::ClassicShoes };
    sf::Color shoeColor{ sf::Color(118, 70, 36) };
    sf::Color shoeSoleColor{ sf::Color(52, 28, 14) };

    // Handwear
    HandwearType handwear{ HandwearType::WhiteCartoon };
    sf::Color gloveColor{ sf::Color(252, 252, 255) };
    sf::Color gloveOutlineColor{ sf::Color(18, 18, 22) };

    // UI & FX Accents
    sf::Color accentColor{ sf::Color(45, 145, 255) };         // Theme color (P1 Azure, P2 Crimson, etc.)
    sf::Color electricColor{ sf::Color(120, 220, 255) };      // Hit spark / EWGF electricity color
    bool glowingEyes{ false };
    sf::Color eyeGlowColor{ sf::Color(120, 220, 255) };

    // Combat Archetype Attributes
    float moveSpeedMult{ 1.0f };
    float damageMult{ 1.0f };
    float defenseMult{ 1.0f };
};

} // namespace RagdollEngine
