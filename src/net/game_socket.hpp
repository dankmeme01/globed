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
struct PacketDataPackResponse {
    int8_t num1;
    int16_t num2;
    int32_t num3;
    int64_t num4;
    uint8_t num5;
    uint16_t num6;
    uint32_t num7;
    uint64_t num8;
    float fl1;
    double fl2;
    std::string str1;
    std::string str2;
};

using RecvPacket = std::variant<PacketCheckedIn, PacketKeepaliveResponse, PacketServerDisconnect, PacketLevelData, PacketDataPackResponse>;

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