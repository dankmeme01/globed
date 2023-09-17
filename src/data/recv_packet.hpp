#pragma once

struct PacketCheckedIn {
    unsigned short tps;
};
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