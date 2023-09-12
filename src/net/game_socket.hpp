#pragma once
#include "bytebuffer.hpp"
#include "udp_socket.hpp"
#include "data_types.hpp"
#include <chrono>
#include <cstdint>
#include <exception>

uint8_t ptToNumber(PacketType pt);
PacketType numberToPt(uint8_t number);

struct PacketCheckedIn {};
struct PacketKeepaliveResponse {
    unsigned int playerCount;
    long long ping;
};
struct PacketServerDisconnect {
    std::string message;
};
struct PacketLevelData {
    std::unordered_map<int, PlayerData> players;
};

using RecvPacket = std::variant<PacketCheckedIn, PacketKeepaliveResponse, PacketServerDisconnect, PacketLevelData>;

void encodePlayerData(const PlayerData& data, ByteBuffer& buffer);
PlayerData decodePlayerData(ByteBuffer& buffer);

class GameSocket : public UdpSocket {
public:
    RecvPacket recvPacket();
    void sendMessage(const Message& message);
    void sendHeartbeat();
    void sendDatapackTest();
    void sendCheckIn();
    void sendDisconnect();
    bool connect(const std::string& serverIp, unsigned short port) override;
    bool close() override;
    void disconnect();

    bool connected;
private:
    std::chrono::high_resolution_clock::time_point keepAliveTime;
};