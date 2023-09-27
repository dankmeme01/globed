#pragma once
#include <stdint.h>

enum class PacketType: uint8_t {
    /* client */
    
    CheckIn = 100,
    Keepalive = 101,
    Disconnect = 102,
    Ping = 103,
    PlayerAccountDataRequest = 104,
    LevelListRequest = 105,
    /* level related */
    UserLevelEntry = 110,
    UserLevelExit = 111,
    UserLevelData = 112,
    SpectateNoData = 113,
    
    /* server */

    CheckedIn = 200,
    KeepaliveResponse = 201, // player count
    ServerDisconnect = 202, // message (string)
    PingResponse = 203,
    PlayerAccountDataResponse = 204,
    LevelListResponse = 205,
    LevelData = 210,
};

uint8_t ptToNumber(PacketType pt);
PacketType numberToPt(uint8_t number);