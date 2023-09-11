#pragma once
#include "net/udp_socket.hpp"
#include <atomic>
#include <variant>
#include <queue>
#include <mutex>
#include <vector>

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

// to send between any thread -> network thread

using Message = std::variant<PlayerData, PlayerEnterLevelData, PlayerDeadData, PlayerLeaveLevelData, GameLoadedData>;

extern std::queue<Message> g_netMsgQueue;
extern std::mutex g_netMutex;
extern std::condition_variable g_netCVar;

// to send playlayer -> playerobject

extern std::atomic_bool g_playerIsPractice;

// general lifecycle

extern std::atomic_bool g_isModLoaded;

extern std::string g_centralURL;
extern std::mutex g_centralMutex;

extern std::queue<std::string> g_errMsgQueue;
extern std::mutex g_errMsgMutex;

extern std::queue<std::string> g_warnMsgQueue;
extern std::mutex g_warnMsgMutex;

extern int g_secretKey;
extern int g_accountID;

extern std::vector<GameServer> g_gameServers;
extern UdpSocket g_gameSocket;
extern std::string g_gameServerId;
