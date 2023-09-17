#include "global_data.hpp"

// to send between any thread -> network thread

SmartMessageQueue<Message> g_netMsgQueue;

// network thread -> playlayer

WrappingMutex<std::unordered_map<int, PlayerData>> g_netRPlayers;

// general lifecycle

std::atomic_bool g_isModLoaded = true;

std::atomic_bool g_shownAccountWarning = false;

WrappingMutex<std::string> g_centralURL;

WrappingMutex<PlayerIconsData> g_iconData;

// sending errors or warnings to ui thread

SmartMessageQueue<std::string> g_errMsgQueue;
SmartMessageQueue<std::string> g_warnMsgQueue;

// game servers

std::atomic_llong g_gameServerPing = -1;
std::atomic_int g_gameServerPlayerCount = 0;
std::atomic_ushort g_gameServerTps;

std::mutex g_gameServerMutex;
std::string g_gameServerId;
std::vector<GameServer> g_gameServers;

WrappingMutex<std::unordered_map<std::string, std::pair<long long, int>>> g_gameServersPings;

// player icon cache

WrappingMutex<std::unordered_map<int, PlayerIconsData>> g_iconCache;

std::shared_ptr<NetworkHandler> g_networkHandler;