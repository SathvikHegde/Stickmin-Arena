#pragma once
#include "NetworkProtocol.hpp"
#include "Fighter.hpp"
#include "CombatManager.hpp"
#include <RagdollEngine/Render/JuiceFX.hpp>

#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/UdpSocket.hpp>
#include <SFML/Network/SocketSelector.hpp>
#include <SFML/System/Clock.hpp>

#include <iostream>
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

    // -------------------------------------------------------------------------
    // CONNECTION MANAGEMENT
    // -------------------------------------------------------------------------
    bool startHost(unsigned short tcpPort = DEFAULT_NET_TCP_PORT, unsigned short udpPort = DEFAULT_NET_UDP_PORT) {
        disconnect();

        m_localTcpPort = tcpPort;
        m_localUdpPort = udpPort;
        m_remoteUdpPort = udpPort;

        if (m_tcpListener.listen(m_localTcpPort) != sf::Socket::Status::Done) {
            m_status = ConnectionStatus::Failed;
            m_statusMessage = "Failed to bind TCP port " + std::to_string(m_localTcpPort);
            return false;
        }
        m_tcpListener.setBlocking(false);

        if (m_udpSocket.bind(m_localUdpPort) != sf::Socket::Status::Done) {
            m_tcpListener.close();
            m_status = ConnectionStatus::Failed;
            m_statusMessage = "Failed to bind UDP port " + std::to_string(m_localUdpPort);
            return false;
        }
        m_udpSocket.setBlocking(false);

        m_role = NetworkRole::Host;
        m_status = ConnectionStatus::Listening;
        m_statusMessage = "Hosting! Waiting for opponent on port " + std::to_string(m_localTcpPort) + "...";
        resetCounters();
        return true;
    }

    bool connectToHost(const std::string& hostIpStr, unsigned short tcpPort = DEFAULT_NET_TCP_PORT, unsigned short udpPort = DEFAULT_NET_UDP_PORT) {
        disconnect();

        auto resolved = sf::IpAddress::resolve(hostIpStr);
        if (!resolved) {
            m_status = ConnectionStatus::Failed;
            m_statusMessage = "Invalid IP or hostname: " + hostIpStr;
            return false;
        }

        m_remoteIp = *resolved;
        m_remoteTcpPort = tcpPort;
        m_remoteUdpPort = udpPort;

        // Bind UDP to any available port
        if (m_udpSocket.bind(sf::Socket::AnyPort) != sf::Socket::Status::Done) {
            m_status = ConnectionStatus::Failed;
            m_statusMessage = "Failed to bind local UDP socket";
            return false;
        }
        m_udpSocket.setBlocking(false);

        m_role = NetworkRole::Client;
        m_status = ConnectionStatus::Connecting;
        m_statusMessage = "Connecting to " + hostIpStr + ":" + std::to_string(tcpPort) + "...";
        resetCounters();

        // Connect with short timeout
        m_tcpSocket.setBlocking(true);
        sf::Socket::Status connectStatus = m_tcpSocket.connect(*m_remoteIp, m_remoteTcpPort, sf::seconds(2.5f));
        m_tcpSocket.setBlocking(false);

        if (connectStatus == sf::Socket::Status::Done) {
            // Send ConnectRequest
            sf::Packet reqPacket;
            reqPacket << static_cast<uint8_t>(PacketType::ConnectRequest)
                      << NET_PROTOCOL_MAGIC << NET_PROTOCOL_VERSION << m_udpSocket.getLocalPort();
            (void)m_tcpSocket.send(reqPacket);

            m_statusMessage = "Connected to host! Synchronizing...";
            return true;
        } else {
            disconnect();
            m_status = ConnectionStatus::Failed;
            m_statusMessage = "Connection failed / timed out to " + hostIpStr;
            return false;
        }
    }

    void disconnect() {
        if (m_status == ConnectionStatus::Connected && m_role != NetworkRole::None) {
            sf::Packet dcPacket;
            dcPacket << static_cast<uint8_t>(PacketType::Disconnect);
            (void)m_tcpSocket.send(dcPacket);
        }

        m_tcpListener.close();
        m_tcpSocket.disconnect();
        m_udpSocket.unbind();

        m_role = NetworkRole::None;
        m_status = ConnectionStatus::Disconnected;
        m_statusMessage = "Disconnected";
        m_remoteIp = std::nullopt;
        resetCounters();
    }

    // -------------------------------------------------------------------------
    // FRAME UPDATE & POLLING
    // -------------------------------------------------------------------------
    void update(float dt) {
        if (m_status == ConnectionStatus::Disconnected || m_status == ConnectionStatus::Failed) return;

        // 1. Host Listening for Client
        if (m_role == NetworkRole::Host && m_status == ConnectionStatus::Listening) {
            if (m_tcpListener.accept(m_tcpSocket) == sf::Socket::Status::Done) {
                m_tcpSocket.setBlocking(false);
                m_remoteIp = m_tcpSocket.getRemoteAddress();
                m_status = ConnectionStatus::Connected;
                m_statusMessage = "Player 2 connected from " + (m_remoteIp ? m_remoteIp->toString() : "client");
                m_lastHeartbeatTimer = 0.0f;
            }
        }

        if (m_status != ConnectionStatus::Connected) return;

        // 2. Poll TCP Packets
        pollTcpMessages();

        // 3. Poll UDP Packets
        pollUdpMessages();

        // 4. Ping / Heartbeat Tracker
        m_pingTimer += dt;
        if (m_pingTimer >= 1.0f) {
            m_pingTimer = 0.0f;
            sendPing();
        }

        m_lastHeartbeatTimer += dt;
        if (m_lastHeartbeatTimer > 10.0f) {
            m_statusMessage = "Connection timed out";
            disconnect();
        }
    }

    // -------------------------------------------------------------------------
    // LOBBY & MATCH CONTROL
    // -------------------------------------------------------------------------
    void sendLobbySync(const LobbySyncData& data) {
        if (!isConnected()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(PacketType::LobbySync) << data;
        (void)m_tcpSocket.send(p);
    }

    bool consumeLobbySync(LobbySyncData& outData) {
        if (m_latestLobbySync) {
            outData = *m_latestLobbySync;
            m_latestLobbySync = std::nullopt;
            return true;
        }
        return false;
    }

    void sendMatchStart() {
        if (!isConnected()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(PacketType::MatchStart);
        (void)m_tcpSocket.send(p);
    }

    bool consumeMatchStart() {
        bool val = m_matchStartReceived;
        m_matchStartReceived = false;
        return val;
    }

    void sendRematch() {
        if (!isConnected()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(PacketType::MatchRematch);
        (void)m_tcpSocket.send(p);
    }

    bool consumeRematch() {
        bool val = m_rematchReceived;
        m_rematchReceived = false;
        return val;
    }

    void sendReturnToLobby() {
        if (!isConnected()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(PacketType::MatchReturnToLobby);
        (void)m_tcpSocket.send(p);
    }

    bool consumeReturnToLobby() {
        bool val = m_returnToLobbyReceived;
        m_returnToLobbyReceived = false;
        return val;
    }

    // -------------------------------------------------------------------------
    // REAL-TIME IN-MATCH DATA (UDP)
    // -------------------------------------------------------------------------
    // Client sends input to Host
    void sendClientInput(const ClientInputData& input) {
        if (!isClient() || !m_remoteIp) return;
        ClientInputData sendData = input;
        sendData.sequence = ++m_inputSequenceOut;

        sf::Packet packet;
        packet << static_cast<uint8_t>(PacketType::ClientInput) << sendData;
        (void)m_udpSocket.send(packet, *m_remoteIp, m_remoteUdpPort);
    }

    // Host consumes received Client input
    bool consumeClientInput(ClientInputData& outInput) {
        if (m_latestClientInput) {
            outInput = *m_latestClientInput;
            m_latestClientInput = std::nullopt;
            return true;
        }
        return false;
    }

    // Host queues visual/audio events to send in snapshot
    void queueJuiceEvent(const JuiceFXEvent& ev) {
        m_pendingOutEvents.push_back(ev);
    }

    // Host broadcasts snapshot to Client
    void sendWorldSnapshot(WorldSnapshotData& snapshot) {
        if (!isHost() || !m_remoteIp) return;
        snapshot.sequence = ++m_snapshotSequenceOut;
        snapshot.events = std::move(m_pendingOutEvents);
        m_pendingOutEvents.clear();

        sf::Packet packet;
        packet << static_cast<uint8_t>(PacketType::WorldSnapshot) << snapshot;
        (void)m_udpSocket.send(packet, *m_remoteIp, m_remoteUdpPort);
    }

    // Client consumes received World snapshot
    bool consumeWorldSnapshot(WorldSnapshotData& outSnapshot) {
        if (m_latestWorldSnapshot) {
            outSnapshot = *m_latestWorldSnapshot;
            m_latestWorldSnapshot = std::nullopt;
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // HIGH-LEVEL GAME SNAPSHOT BUILD & APPLY HELPERS
    // -------------------------------------------------------------------------
    WorldSnapshotData buildSnapshot(const CombatManager& combat, Fighter& p1, Fighter& p2) {
        WorldSnapshotData snap;
        snap.matchState = static_cast<uint8_t>(combat.getState());
        snap.roundTimer = combat.getRoundTimer();
        snap.roundNumber = static_cast<uint8_t>(combat.getRoundNumber());
        snap.roundWinner = static_cast<uint8_t>(combat.getRoundWinner());
        snap.p1RoundsWon = static_cast<uint8_t>(p1.getRoundsWon());
        snap.p2RoundsWon = static_cast<uint8_t>(p2.getRoundsWon());

        // P1 Snapshot
        snap.p1.health = p1.getHealth();
        snap.p1.ghostHealth = p1.getGhostHealth();
        snap.p1.facingDir = static_cast<int8_t>(p1.getController()->getFacingDirection());
        snap.p1.actionState = static_cast<uint8_t>(p1.getController()->getActionState());
        snap.p1.currentMoveId = static_cast<uint8_t>(p1.getController()->getCurrentMove().id);
        snap.p1.rageArtUsed = p1.hasUsedRageArt();
        snap.p1.comboHits = static_cast<int8_t>(p1.getComboHits());
        snap.p1.comboDamage = p1.getComboDamage();
        snap.p1.limbs = p1.getSkeleton()->getLimbTransforms();

        // P2 Snapshot
        snap.p2.health = p2.getHealth();
        snap.p2.ghostHealth = p2.getGhostHealth();
        snap.p2.facingDir = static_cast<int8_t>(p2.getController()->getFacingDirection());
        snap.p2.actionState = static_cast<uint8_t>(p2.getController()->getActionState());
        snap.p2.currentMoveId = static_cast<uint8_t>(p2.getController()->getCurrentMove().id);
        snap.p2.rageArtUsed = p2.hasUsedRageArt();
        snap.p2.comboHits = static_cast<int8_t>(p2.getComboHits());
        snap.p2.comboDamage = p2.getComboDamage();
        snap.p2.limbs = p2.getSkeleton()->getLimbTransforms();

        return snap;
    }

    void applySnapshot(const WorldSnapshotData& snap, CombatManager& combat, Fighter& p1, Fighter& p2, RagdollEngine::JuiceFX& juiceFX) {
        combat.setState(static_cast<MatchState>(snap.matchState));
        combat.setRoundTimer(snap.roundTimer);
        combat.setRoundNumber(snap.roundNumber);
        combat.setRoundWinner(snap.roundWinner);

        p1.setHealth(snap.p1.health);
        p1.setGhostHealth(snap.p1.ghostHealth);
        p1.setRoundsWon(snap.p1RoundsWon);
        p1.setComboHits(snap.p1.comboHits);
        p1.setComboDamage(snap.p1.comboDamage);
        p1.getController()->setFacingDirection(snap.p1.facingDir);
        p1.setRageArtUsed(snap.p1.rageArtUsed);
        p1.getSkeleton()->setLimbTransforms(snap.p1.limbs);

        p2.setHealth(snap.p2.health);
        p2.setGhostHealth(snap.p2.ghostHealth);
        p2.setRoundsWon(snap.p2RoundsWon);
        p2.setComboHits(snap.p2.comboHits);
        p2.setComboDamage(snap.p2.comboDamage);
        p2.getController()->setFacingDirection(snap.p2.facingDir);
        p2.setRageArtUsed(snap.p2.rageArtUsed);
        p2.getSkeleton()->setLimbTransforms(snap.p2.limbs);

        // Process network Juice events
        for (const auto& ev : snap.events) {
            sf::Color col(ev.colorR, ev.colorG, ev.colorB);
            if (ev.type == JuiceEventType::ImpactHit) {
                juiceFX.spawnImpact(sf::Vector2f(ev.posX, ev.posY), sf::Vector2f(ev.normalX, ev.normalY), col, false);
            } else if (ev.type == JuiceEventType::ElectricBurst) {
                juiceFX.spawnElectricBurst(sf::Vector2f(ev.posX, ev.posY), col, 4, 32.0f);
            } else if (ev.type == JuiceEventType::FloatingText) {
                juiceFX.spawnFloatingText(sf::Vector2f(ev.posX, ev.posY), ev.text, col, 1.6f);
            } else if (ev.type == JuiceEventType::ScreenFlash) {
                juiceFX.triggerScreenFlash(0.14f, col);
            }
        }
    }

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
    bool m_matchStartReceived{ false };
    bool m_rematchReceived{ false };
    bool m_returnToLobbyReceived{ false };
    std::optional<ClientInputData> m_latestClientInput;
    std::optional<WorldSnapshotData> m_latestWorldSnapshot;

    // Outbound queues
    std::vector<JuiceFXEvent> m_pendingOutEvents;

    void resetCounters() {
        m_inputSequenceOut = 0;
        m_inputSequenceIn = 0;
        m_snapshotSequenceOut = 0;
        m_snapshotSequenceIn = 0;
        m_pingTimer = 0.0f;
        m_pingMs = 0.0f;
        m_lastHeartbeatTimer = 0.0f;
        m_latestLobbySync = std::nullopt;
        m_matchStartReceived = false;
        m_rematchReceived = false;
        m_returnToLobbyReceived = false;
        m_latestClientInput = std::nullopt;
        m_latestWorldSnapshot = std::nullopt;
        m_pendingOutEvents.clear();
    }

    void sendPing() {
        if (!isConnected()) return;
        m_pingSendTime = std::chrono::steady_clock::now();
        sf::Packet p;
        p << static_cast<uint8_t>(PacketType::Ping);
        (void)m_tcpSocket.send(p);
    }

    void pollTcpMessages() {
        sf::Packet packet;
        while (m_tcpSocket.receive(packet) == sf::Socket::Status::Done) {
            m_lastHeartbeatTimer = 0.0f;

            uint8_t rawType = 0;
            if (!(packet >> rawType)) continue;

            auto type = static_cast<PacketType>(rawType);
            switch (type) {
                case PacketType::ConnectRequest: {
                    uint32_t magic = 0;
                    uint16_t version = 0;
                    unsigned short clientUdpPort = DEFAULT_NET_UDP_PORT;
                    if (packet >> magic >> version >> clientUdpPort) {
                        if (magic == NET_PROTOCOL_MAGIC && version == NET_PROTOCOL_VERSION) {
                            m_remoteUdpPort = clientUdpPort;
                            sf::Packet acceptPacket;
                            acceptPacket << static_cast<uint8_t>(PacketType::ConnectAccept) << m_udpSocket.getLocalPort();
                            (void)m_tcpSocket.send(acceptPacket);
                        } else {
                            sf::Packet rejectPacket;
                            rejectPacket << static_cast<uint8_t>(PacketType::ConnectReject);
                            (void)m_tcpSocket.send(rejectPacket);
                            disconnect();
                        }
                    }
                    break;
                }
                case PacketType::ConnectAccept: {
                    unsigned short hostUdpPort = DEFAULT_NET_UDP_PORT;
                    if (packet >> hostUdpPort) {
                        m_remoteUdpPort = hostUdpPort;
                    }
                    m_status = ConnectionStatus::Connected;
                    m_statusMessage = "Connected to host!";
                    break;
                }
                case PacketType::ConnectReject: {
                    disconnect();
                    m_status = ConnectionStatus::Failed;
                    m_statusMessage = "Connection rejected by host (protocol mismatch)";
                    break;
                }
                case PacketType::Disconnect: {
                    disconnect();
                    m_statusMessage = "Opponent disconnected";
                    break;
                }
                case PacketType::Ping: {
                    sf::Packet pong;
                    pong << static_cast<uint8_t>(PacketType::Pong);
                    (void)m_tcpSocket.send(pong);
                    break;
                }
                case PacketType::Pong: {
                    auto now = std::chrono::steady_clock::now();
                    std::chrono::duration<float, std::milli> elapsed = now - m_pingSendTime;
                    m_pingMs = elapsed.count();
                    break;
                }
                case PacketType::LobbySync: {
                    LobbySyncData lobbyData;
                    if (packet >> lobbyData) {
                        m_latestLobbySync = lobbyData;
                    }
                    break;
                }
                case PacketType::MatchStart: {
                    m_matchStartReceived = true;
                    break;
                }
                case PacketType::MatchRematch: {
                    m_rematchReceived = true;
                    break;
                }
                case PacketType::MatchReturnToLobby: {
                    m_returnToLobbyReceived = true;
                    break;
                }
                default:
                    break;
            }
        }
    }

    void pollUdpMessages() {
        sf::Packet packet;
        std::optional<sf::IpAddress> senderIp;
        unsigned short senderPort = 0;

        while (m_udpSocket.receive(packet, senderIp, senderPort) == sf::Socket::Status::Done) {
            uint8_t rawType = 0;
            if (!(packet >> rawType)) continue;

            auto type = static_cast<PacketType>(rawType);
            if (m_role == NetworkRole::Host && type == PacketType::ClientInput) {
                ClientInputData input;
                if (packet >> input) {
                    if (input.sequence > m_inputSequenceIn) {
                        m_inputSequenceIn = input.sequence;
                        m_latestClientInput = input;
                    }
                }
            } else if (m_role == NetworkRole::Client && type == PacketType::WorldSnapshot) {
                WorldSnapshotData snap;
                if (packet >> snap) {
                    if (snap.sequence > m_snapshotSequenceIn) {
                        m_snapshotSequenceIn = snap.sequence;
                        m_latestWorldSnapshot = std::move(snap);
                    }
                }
            }
        }
    }
};

} // namespace StickminGame
