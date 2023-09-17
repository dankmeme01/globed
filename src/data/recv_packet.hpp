#pragma once
#include "player_data.hpp"
#include "player_account_data.hpp"

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

struct PacketAccountDataResponse {
    int playerId;
    PlayerAccountData data;
};

using RecvPacket = std::variant<PacketCheckedIn, PacketKeepaliveResponse, PacketServerDisconnect, PacketLevelData, PacketPingResponse, PacketAccountDataResponse>;