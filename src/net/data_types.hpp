#pragma once

enum class IconGameMode : uint8_t {
    CUBE = 0,
    SHIP = 1,
    BALL = 2,
    UFO = 3,
    WAVE = 4,
    ROBOT = 5,
    SPIDER = 6,
};

struct SpecificIconData {
    float x;
    float y;
    float xRot;
    float yRot;
    IconGameMode gameMode;

    bool isHidden;
    bool isDashing;
    bool isUpsideDown;
};

struct PlayerData {
    SpecificIconData player1;
    SpecificIconData player2;
    
    bool isPractice;
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