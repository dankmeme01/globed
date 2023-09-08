#pragma once
#include <atomic>
#include <variant>
#include <queue>
#include <mutex>

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

// to send between any thread -> network thread

using Message = std::variant<PlayerData, PlayerEnterLevelData, PlayerDeadData, PlayerLeaveLevelData>;

extern std::queue<Message> g_netMsgQueue;
extern std::mutex g_netMutex;
extern std::condition_variable g_netCVar;

// to send playlayer -> playerobject

extern std::atomic_bool g_playerIsPractice;

// general lifecycle

extern std::atomic_bool g_isModLoaded;