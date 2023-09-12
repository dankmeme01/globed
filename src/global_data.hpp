#pragma once
#include "net/game_socket.hpp"
#include "net/udp_socket.hpp"
#include "net/data_types.hpp"
#include <atomic>
#include <unordered_map>
#include <variant>
#include <queue>
#include <mutex>
#include <vector>

// to send between any thread -> network thread

extern std::queue<Message> g_netMsgQueue;
extern std::mutex g_netMutex;

// network thread -> playlayer

extern std::unordered_map<int, PlayerData> g_netRPlayers;
extern std::mutex g_netRMutex;

// to send playlayer -> playerobject

extern std::atomic_bool g_playerIsPractice;

// general lifecycle

extern bool g_isModLoaded;
extern std::mutex g_modLoadedMutex;
extern std::condition_variable g_modLoadedCv;

extern std::atomic_bool g_shownAccountWarning;

extern std::string g_centralURL;
extern std::mutex g_centralMutex;

extern int g_secretKey;
extern int g_accountID;

// sending errors or warnings to ui thread

extern std::queue<std::string> g_errMsgQueue;
extern std::mutex g_errMsgMutex;

extern std::queue<std::string> g_warnMsgQueue;
extern std::mutex g_warnMsgMutex;

// game servers

extern std::mutex g_gameServerMutex;
extern std::vector<GameServer> g_gameServers;
extern std::string g_gameServerId;
extern GameSocket g_gameSocket;
extern long long g_gameServerPing;
extern int g_gameServerPlayerCount;
extern std::unordered_map<std::string, std::pair<long long, int>> g_gameServersPings;
