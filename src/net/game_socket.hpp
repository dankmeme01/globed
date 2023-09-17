#pragma once
#include "bytebuffer.hpp"
#include "udp_socket.hpp"
#include "../data/data.hpp"
#include <chrono>
#include <cstdint>
#include <exception>

class GameSocket : public UdpSocket {
public:
    GameSocket(int _accountId, int _secretKey);
    RecvPacket recvPacket();
    void sendMessage(const Message& message);
    void sendHeartbeat();
    void sendCheckIn();
    void sendAccountDataRequest(int playerId);
    void sendDisconnect();
    void sendPingTo(const std::string& serverId, const std::string& serverIp, unsigned short port);
    void disconnect() override;
    bool established = false;
    int accountId, secretKey;
private:
    void sendBuf(const ByteBuffer& buf);
    void writeAuth(ByteBuffer& buf);
    std::mutex sendMutex;
    std::chrono::high_resolution_clock::time_point keepAliveTime;
    std::unordered_map<int, std::pair<std::string, std::chrono::system_clock::time_point>> pingTimes;
};