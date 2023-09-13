#include "global_data.hpp"

// to send between any thread -> network thread

WrappingMutex<std::queue<Message>> g_netMsgQueue;

// network thread -> playlayer

WrappingMutex<std::unordered_map<int, PlayerData>> g_netRPlayers;

// general lifecycle

bool g_isModLoaded = true;
std::mutex g_modLoadedMutex;
std::condition_variable g_modLoadedCv;

std::atomic_bool g_shownAccountWarning = false;

WrappingMutex<std::string> g_centralURL;

int g_secretKey = 0;
int g_accountID = 0;

// sending errors or warnings to ui thread

WrappingMutex<std::queue<std::string>> g_errMsgQueue;
WrappingMutex<std::queue<std::string>> g_warnMsgQueue;

// game servers

std::mutex g_gameServerMutex;
std::string g_gameServerId;
std::vector<GameServer> g_gameServers;
GameSocket g_gameSocket;
long long g_gameServerPing;
int g_gameServerPlayerCount;
std::unordered_map<std::string, std::pair<long long, int>> g_gameServersPings;