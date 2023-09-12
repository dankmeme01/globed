#include "global_data.hpp"

std::queue<Message> g_netMsgQueue;
std::mutex g_netMutex;

std::unordered_map<int, PlayerData> g_netRPlayers;
std::mutex g_netRMutex;

std::atomic_bool g_playerIsPractice = false;

bool g_isModLoaded = true;
std::mutex g_modLoadedMutex;
std::condition_variable g_modLoadedCv;

std::atomic_bool g_shownAccountWarning = false;

std::string g_centralURL = "";
std::mutex g_centralMutex;

int g_secretKey = 0;
int g_accountID = 0;


std::queue<std::string> g_errMsgQueue;
std::mutex g_errMsgMutex;

std::queue<std::string> g_warnMsgQueue;
std::mutex g_warnMsgMutex;


std::mutex g_gameServerMutex;
std::string g_gameServerId;
std::vector<GameServer> g_gameServers;
GameSocket g_gameSocket;
long long g_gameServerPing;
int g_gameServerPlayerCount;
std::unordered_map<std::string, std::pair<long long, int>> g_gameServersPings;