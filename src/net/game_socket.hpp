#pragma once
#include "bytebuffer.hpp"
#include "udp_socket.hpp"
#include "data_types.hpp"
#include <chrono>
#include <cstdint>
#include <exception>

// https://stackoverflow.com/questions/8357240/how-to-automatically-convert-strongly-typed-enum-into-int
template <typename E>
constexpr typename std::underlying_type<E>::type toUnderlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

uint8_t ptToNumber(PacketType pt);
PacketType numberToPt(uint8_t number);

uint8_t gmToNumber(IconGameMode gm);
IconGameMode numberToGm(uint8_t number);

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
struct PacketPingResponse {
    std::string serverId;
    unsigned int playerCount;
    long long ping;
};

using RecvPacket = std::variant<PacketCheckedIn, PacketKeepaliveResponse, PacketServerDisconnect, PacketLevelData, PacketPingResponse>;

void encodeSpecificIcon(const SpecificIconData& data, ByteBuffer& buffer);
void encodePlayerData(const PlayerData& data, ByteBuffer& buffer);

SpecificIconData decodeSpecificIcon(ByteBuffer& buffer);
PlayerData decodePlayerData(ByteBuffer& buffer);

class GameSocket : public UdpSocket {
public:
    GameSocket(int _accountId, int _secretKey);
    RecvPacket recvPacket();
    void sendMessage(const Message& message);
    void sendHeartbeat();
    void sendDatapackTest();
    void sendCheckIn();
    void sendDisconnect();
    void sendPingTo(const std::string& serverId, const std::string& serverIp, unsigned short port);
    // bool connect(const std::string& serverIp, unsigned short port) override;
    void disconnect() override;
    bool established = false;
    int accountId, secretKey;
private:
    std::mutex sendMutex;
    std::chrono::high_resolution_clock::time_point keepAliveTime;
    std::unordered_map<int, std::pair<std::string, std::chrono::system_clock::time_point>> pingTimes;
};