#pragma once
#include "player_data.hpp"

struct PlayerEnterLevelData {
    int levelID;
};

struct PlayerDeadData {};

struct PlayerLeaveLevelData {};

struct GameLoadedData {};

struct GameServer {
    std::string name;
    std::string id;
    std::string region;
    std::string address;
};

struct PingServers {};

struct CentralServerChanged {
    std::string server;
};

using Message = std::variant<PlayerData, PlayerEnterLevelData, PlayerDeadData, PlayerLeaveLevelData, GameLoadedData, PingServers, CentralServerChanged>;