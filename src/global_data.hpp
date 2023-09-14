#pragma once
#include "net/network_handler.hpp"
#include "net/udp_socket.hpp"
#include "net/data_types.hpp"
#include "wrapping_mutex.hpp"
#include <atomic>
#include <unordered_map>
#include <variant>
#include <queue>
#include <mutex>
#include <vector>

// to send between any thread -> network thread

extern WrappingMutex<std::queue<Message>> g_netMsgQueue;

// network thread -> playlayer

extern WrappingMutex<std::unordered_map<int, PlayerData>> g_netRPlayers;

// general lifecycle

extern std::atomic_bool g_isModLoaded;

extern std::atomic_bool g_shownAccountWarning;

extern WrappingMutex<std::string> g_centralURL;

extern int g_secretKey;
extern int g_accountID;

// sending errors or warnings to ui thread

extern WrappingMutex<std::queue<std::string>> g_errMsgQueue;
extern WrappingMutex<std::queue<std::string>> g_warnMsgQueue;

// game servers

extern std::atomic_llong g_gameServerPing;
extern std::atomic_int g_gameServerPlayerCount;

extern std::mutex g_gameServerMutex;
extern std::vector<GameServer> g_gameServers;
extern std::string g_gameServerId;

extern WrappingMutex<std::unordered_map<std::string, std::pair<long long, int>>> g_gameServersPings;

extern std::shared_ptr<NetworkHandler> g_networkHandler; // this should be destructed first
