#pragma once
#include "player_data.hpp"
#include "player_icons.hpp"

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

struct PacketPlayerIconsResponse {
    int playerId;
    PlayerIconsData icons;
};

using RecvPacket = std::variant<PacketCheckedIn, PacketKeepaliveResponse, PacketServerDisconnect, PacketLevelData, PacketPingResponse, PacketPlayerIconsResponse>;