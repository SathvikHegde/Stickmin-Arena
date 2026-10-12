#pragma once
#include "NetworkProtocol.hpp"
#include "Fighter.hpp"
#include "CombatManager.hpp"
#include <RagdollEngine/Render/JuiceFX.hpp>
#include <RagdollEngine/Render/StageRenderer.hpp>
#include <RagdollEngine/Core/CameraDirector.hpp>

#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/UdpSocket.hpp>
#include <SFML/Network/SocketSelector.hpp>
#include <SFML/System/Clock.hpp>

#include <string>
#include <optional>
#include <vector>
#include <chrono>

namespace StickminGame {

enum class NetworkRole {
    None,
    Host,
    Client
};

enum class ConnectionStatus {
    Disconnected,
    Listening,
    Connecting,
    Connected,
    Failed
};

class NetworkManager {
public:
    NetworkManager() = default;
    ~NetworkManager() {
        disconnect();
    }

    NetworkRole getRole() const { return m_role; }
    ConnectionStatus getStatus() const { return m_status; }
    bool isConnected() const { return m_status == ConnectionStatus::Connected; }
    bool isHost() const { return m_role == NetworkRole::Host && isConnected(); }
    bool isClient() const { return m_role == NetworkRole::Client && isConnected(); }
    const std::string& getStatusMessage() const { return m_statusMessage; }
    float getPingMs() const { return m_pingMs; }
    std::string getLocalIpString() const {
        auto addr = sf::IpAddress::getLocalAddress();
        return addr ? addr->toString() : "127.0.0.1";
    }

    // Connection management
    bool startHost(unsigned short tcpPort = DEFAULT_NET_TCP_PORT, unsigned short udpPort = DEFAULT_NET_UDP_PORT);
    bool connectToHost(const std::string& hostIpStr, unsigned short tcpPort = DEFAULT_NET_TCP_PORT, unsigned short udpPort = DEFAULT_NET_UDP_PORT);
    void disconnect();

    // Frame update & polling
    void update(float dt);

    // Lobby & Match control
    void sendLobbySync(const LobbySyncData& data);
    bool consumeLobbySync(LobbySyncData& outData);
    void sendMatchStart(uint8_t stageIdx = 0, uint8_t p1CharIdx = 0, uint8_t p2CharIdx = 1);
    bool consumeMatchStart(MatchStartData& outData);
    void sendRematch();
    bool consumeRematch();
    void sendReturnToLobby();
    bool consumeReturnToLobby();

    // Real-time in-match UDP
    void sendClientInput(const ClientInputData& input);
    bool consumeClientInput(ClientInputData& outInput);
    void queueJuiceEvent(const JuiceFXEvent& ev);
    void sendWorldSnapshot(WorldSnapshotData& snapshot);
    bool consumeWorldSnapshot(WorldSnapshotData& outSnapshot);

    // High-level game snapshot build & apply
    WorldSnapshotData buildSnapshot(const CombatManager& combat, Fighter& p1, Fighter& p2, uint8_t stageIdx = 0);
    void applySnapshot(const WorldSnapshotData& snap, CombatManager& combat, Fighter& p1, Fighter& p2, RagdollEngine::JuiceFX& juiceFX, RagdollEngine::CameraDirector* camera = nullptr, RagdollEngine::StageRenderer* stageRenderer = nullptr);

private:
    NetworkRole m_role{ NetworkRole::None };
    ConnectionStatus m_status{ ConnectionStatus::Disconnected };
    std::string m_statusMessage{ "Disconnected" };

    sf::TcpListener m_tcpListener;
    sf::TcpSocket m_tcpSocket;
    sf::UdpSocket m_udpSocket;

    std::optional<sf::IpAddress> m_remoteIp;
    unsigned short m_remoteTcpPort{ DEFAULT_NET_TCP_PORT };
    unsigned short m_remoteUdpPort{ DEFAULT_NET_UDP_PORT };
    unsigned short m_localTcpPort{ DEFAULT_NET_TCP_PORT };
    unsigned short m_localUdpPort{ DEFAULT_NET_UDP_PORT };

    // Sequence tracking
    uint32_t m_inputSequenceOut{ 0 };
    uint32_t m_inputSequenceIn{ 0 };
    uint32_t m_snapshotSequenceOut{ 0 };
    uint32_t m_snapshotSequenceIn{ 0 };

    // Ping & Heartbeat
    float m_pingTimer{ 0.0f };
    float m_pingMs{ 0.0f };
    float m_lastHeartbeatTimer{ 0.0f };
    std::chrono::steady_clock::time_point m_pingSendTime;

    // Inbound queues
    std::optional<LobbySyncData> m_latestLobbySync;
    std::optional<MatchStartData> m_latestMatchStart;
    bool m_rematchReceived{ false };
    bool m_returnToLobbyReceived{ false };
    std::optional<ClientInputData> m_latestClientInput;
    std::optional<WorldSnapshotData> m_latestWorldSnapshot;

    // Outbound queues
    std::vector<JuiceFXEvent> m_pendingOutEvents;

    void resetCounters();
    void sendPing();
    void pollTcpMessages();
    void pollUdpMessages();
};

} // namespace StickminGame
