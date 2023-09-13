#pragma once

struct PlayerData {
    bool isPractice;
    float x;
    float y;
    float xRot;
    float yRot;
    bool isHidden;
    bool isDashing;

    cocos2d::ccColor3B playerColor1;
    cocos2d::ccColor3B playerColor2;
};

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

using Message = std::variant<PlayerData, PlayerEnterLevelData, PlayerDeadData, PlayerLeaveLevelData, GameLoadedData, PingServers>;

enum class PacketType: uint8_t {
    /* client */
    
    CheckIn = 100,
    Keepalive = 101,
    Disconnect = 102,
    Ping = 103,
    /* level related */
    UserLevelEntry = 110,
    UserLevelExit = 111,
    UserLevelData = 112,

    /* server */

    CheckedIn = 200,
    KeepaliveResponse = 201, // player count
    ServerDisconnect = 202, // message (string)
    PingResponse = 203,
    LevelData = 210,
};