#include "global_data.hpp"

bool g_debug;

// to send between any thread -> network thread

SmartMessageQueue<NetworkThreadMessage> g_netMsgQueue;

// network thread -> playlayer

PlayerCorrector g_pCorrector;

// general lifecycle

std::atomic_bool g_isModLoaded = true;

std::atomic_bool g_shownAccountWarning = false;

WrappingMutex<std::string> g_centralURL;

WrappingMutex<PlayerAccountData> g_accountData;

// sending errors or warnings to ui thread

SmartMessageQueue<std::string> g_errMsgQueue;
SmartMessageQueue<std::string> g_warnMsgQueue;

// game servers

std::atomic_llong g_gameServerPing = -1;
std::atomic_int g_gameServerPlayerCount = 0;
std::atomic_ushort g_gameServerTps;
std::atomic_llong g_gameServerLastHeartbeat = 0;

std::mutex g_gameServerMutex;
std::string g_gameServerId;
std::vector<GameServer> g_gameServers;

WrappingMutex<std::unordered_map<std::string, std::pair<long long, int>>> g_gameServersPings;

// level list

std::atomic_bool g_levelsLoading = false;
WrappingMutex<std::unordered_map<int, unsigned short>> g_levelsList;

// level cache

WrappingMutex<std::unordered_map<int, GJGameLevel*>> g_levelDataCache;

// player icon cache

WrappingMutex<std::unordered_map<int, PlayerAccountData>> g_accDataCache;

// the spectated player, play layer -> spectate user cell
std::atomic_int g_spectatedPlayer = 0;

// current level id, for hiding the players button on PauseLayer
std::atomic_int g_currentLevelId = 0;

std::shared_ptr<NetworkHandler> g_networkHandler;