#pragma once

struct PlayerData {
    bool isPractice;
    float x;
    float y;
    bool isHidden;
    bool isUpsideDown;
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

using Message = std::variant<PlayerData, PlayerEnterLevelData, PlayerDeadData, PlayerLeaveLevelData, GameLoadedData>;

enum class PacketType: uint8_t {
    /* client */
    
    CheckIn = 100,
    Keepalive = 101,
    Disconnect = 102,
    /* level related */
    UserLevelEntry = 110,
    UserLevelExit = 111,
    UserLevelData = 112,

    /* server */

    CheckedIn = 200,
    KeepaliveResponse = 201, // player count
    ServerDisconnect = 202, // message (string)
    LevelData = 210,
};