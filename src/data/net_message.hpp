/*
* Message is for sending things between main thread and network thread, via SmartMessageQueue.
*/

#pragma once
#include "player_data.hpp"

struct NMPlayerLevelEntry {
    int levelID;
};

struct NMPlayerDied {};

struct NMPlayerLevelExit {};

struct NMMenuLayerEntry {};

struct NMPingServers {};

struct NMCentralServerChanged {
    std::string server;
};

struct NMRequestPlayerAccount {
    int playerId;
};

struct NMRequestLevelList {};

using NetworkThreadMessage = std::variant<
    PlayerData,
    NMPlayerLevelEntry,
    NMPlayerDied,
    NMPlayerLevelExit,
    NMMenuLayerEntry,
    NMPingServers,
    NMCentralServerChanged,
    NMRequestPlayerAccount,
    NMRequestLevelList
>;