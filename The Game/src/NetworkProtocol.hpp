#pragma once
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <RagdollEngine/Physics/RagdollSkeleton.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace StickminGame {

constexpr uint32_t NET_PROTOCOL_MAGIC = 0x53544B4D; // 'STKM'
constexpr uint16_t NET_PROTOCOL_VERSION = 1;
constexpr unsigned short DEFAULT_NET_TCP_PORT = 24800;
constexpr unsigned short DEFAULT_NET_UDP_PORT = 24801;

enum class PacketType : uint8_t {
    None = 0,
    ConnectRequest = 1,
    ConnectAccept = 2,
    ConnectReject = 3,
    Disconnect = 4,
    Ping = 5,
    Pong = 6,
    LobbySync = 7,
    MatchStart = 8,
    MatchRematch = 9,
    MatchReturnToLobby = 10,
    ClientInput = 20,
    WorldSnapshot = 21
};

namespace InputButtons {
    constexpr uint16_t LP         = 1 << 0;  // 1 (Left Punch / Flash Jab)
    constexpr uint16_t RP         = 1 << 1;  // 2 (Right Punch / Straight Cross)
    constexpr uint16_t LK         = 1 << 2;  // 3 (Left Kick / Mid Kick)
    constexpr uint16_t RK         = 1 << 3;  // 4 (Right Kick / Axe Roundhouse)
    constexpr uint16_t PowerCrush = 1 << 4;  // 1+2 (Power Crush / Rage Art)
    constexpr uint16_t Throw      = 1 << 5;  // 1+3 (Command Throw)
    constexpr uint16_t Dropkick   = 1 << 6;  // 3+4 (Flying Dropkick)
    constexpr uint16_t Jump       = 1 << 7;  // Jump (W or Space)
}

struct ClientInputData {
    uint32_t sequence{ 0 };
    float moveX{ 0.0f };
    float moveY{ 0.0f };
    uint16_t buttons{ 0 };
    int8_t dashDir{ 0 }; // -1 = back dash, 0 = none, +1 = forward dash
};

inline sf::Packet& operator<<(sf::Packet& packet, const ClientInputData& input) {
    return packet << input.sequence << input.moveX << input.moveY << input.buttons << input.dashDir;
}

inline sf::Packet& operator>>(sf::Packet& packet, ClientInputData& input) {
    return packet >> input.sequence >> input.moveX >> input.moveY >> input.buttons >> input.dashDir;
}

struct LobbySyncData {
    uint8_t p1CharIdx{ 0 };
    uint8_t p2CharIdx{ 1 };
    uint8_t stageIdx{ 0 };
    bool p1Ready{ false };
    bool p2Ready{ false };
};

inline sf::Packet& operator<<(sf::Packet& packet, const LobbySyncData& data) {
    return packet << data.p1CharIdx << data.p2CharIdx << data.stageIdx << data.p1Ready << data.p2Ready;
}

inline sf::Packet& operator>>(sf::Packet& packet, LobbySyncData& data) {
    return packet >> data.p1CharIdx >> data.p2CharIdx >> data.stageIdx >> data.p1Ready >> data.p2Ready;
}

enum class JuiceEventType : uint8_t {
    None = 0,
    ImpactHit = 1,
    ElectricBurst = 2,
    FloatingText = 3,
    ScreenFlash = 4,
    CameraTrauma = 5
};

struct JuiceFXEvent {
    JuiceEventType type{ JuiceEventType::None };
    float posX{ 0.0f };
    float posY{ 0.0f };
    float normalX{ 0.0f };
    float normalY{ 0.0f };
    uint8_t colorR{ 255 };
    uint8_t colorG{ 255 };
    uint8_t colorB{ 255 };
    float intensity{ 1.0f };
    std::string text;
};

inline sf::Packet& operator<<(sf::Packet& packet, const JuiceFXEvent& e) {
    uint8_t t = static_cast<uint8_t>(e.type);
    return packet << t << e.posX << e.posY << e.normalX << e.normalY
                  << e.colorR << e.colorG << e.colorB << e.intensity << e.text;
}

inline sf::Packet& operator>>(sf::Packet& packet, JuiceFXEvent& e) {
    uint8_t t = 0;
    packet >> t >> e.posX >> e.posY >> e.normalX >> e.normalY
           >> e.colorR >> e.colorG >> e.colorB >> e.intensity >> e.text;
    e.type = static_cast<JuiceEventType>(t);
    return packet;
}

struct FighterSnapshotData {
    float health{ 100.0f };
    float ghostHealth{ 100.0f };
    int8_t facingDir{ 1 };
    uint8_t actionState{ 0 };
    uint8_t currentMoveId{ 0 };
    bool rageArtUsed{ false };
    int8_t comboHits{ 0 };
    float comboDamage{ 0.0f };
    RagdollEngine::SkeletonTransforms limbs{};
};

inline sf::Packet& operator<<(sf::Packet& packet, const FighterSnapshotData& f) {
    packet << f.health << f.ghostHealth << f.facingDir << f.actionState << f.currentMoveId
           << f.rageArtUsed << f.comboHits << f.comboDamage;
    for (const auto& limb : f.limbs) {
        packet << limb.position.x << limb.position.y << limb.angleRadians;
    }
    return packet;
}

inline sf::Packet& operator>>(sf::Packet& packet, FighterSnapshotData& f) {
    packet >> f.health >> f.ghostHealth >> f.facingDir >> f.actionState >> f.currentMoveId
           >> f.rageArtUsed >> f.comboHits >> f.comboDamage;
    for (auto& limb : f.limbs) {
        packet >> limb.position.x >> limb.position.y >> limb.angleRadians;
    }
    return packet;
}

struct MatchStartData {
    uint8_t stageIdx{ 0 };
    uint8_t p1CharIdx{ 0 };
    uint8_t p2CharIdx{ 1 };
};

inline sf::Packet& operator<<(sf::Packet& packet, const MatchStartData& d) {
    return packet << d.stageIdx << d.p1CharIdx << d.p2CharIdx;
}

inline sf::Packet& operator>>(sf::Packet& packet, MatchStartData& d) {
    return packet >> d.stageIdx >> d.p1CharIdx >> d.p2CharIdx;
}

struct WorldSnapshotData {
    uint32_t sequence{ 0 };
    uint8_t matchState{ 0 }; // Cast to/from MatchState
    float roundTimer{ 60.0f };
    uint8_t roundNumber{ 1 };
    uint8_t roundWinner{ 0 };
    uint8_t p1RoundsWon{ 0 };
    uint8_t p2RoundsWon{ 0 };
    uint8_t stageIdx{ 0 };
    FighterSnapshotData p1;
    FighterSnapshotData p2;
    std::vector<JuiceFXEvent> events;
};

inline sf::Packet& operator<<(sf::Packet& packet, const WorldSnapshotData& s) {
    packet << s.sequence << s.matchState << s.roundTimer << s.roundNumber << s.roundWinner
           << s.p1RoundsWon << s.p2RoundsWon << s.stageIdx << s.p1 << s.p2;
    uint16_t numEvents = static_cast<uint16_t>(s.events.size());
    packet << numEvents;
    for (const auto& ev : s.events) {
        packet << ev;
    }
    return packet;
}

inline sf::Packet& operator>>(sf::Packet& packet, WorldSnapshotData& s) {
    packet >> s.sequence >> s.matchState >> s.roundTimer >> s.roundNumber >> s.roundWinner
           >> s.p1RoundsWon >> s.p2RoundsWon >> s.stageIdx >> s.p1 >> s.p2;
    uint16_t numEvents = 0;
    packet >> numEvents;
    s.events.resize(numEvents);
    for (uint16_t i = 0; i < numEvents; ++i) {
        packet >> s.events[i];
    }
    return packet;
}

} // namespace StickminGame
