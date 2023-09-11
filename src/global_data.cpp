#include "global_data.hpp"

std::queue<Message> g_netMsgQueue;
std::mutex g_netMutex;
std::condition_variable g_netCVar;

std::atomic_bool g_playerIsPractice = false;

std::atomic_bool g_isModLoaded = true;

std::string g_centralURL = "";
std::mutex g_centralMutex;

std::queue<std::string> g_errMsgQueue;
std::mutex g_errMsgMutex;

std::queue<std::string> g_warnMsgQueue;
std::mutex g_warnMsgMutex;

int g_secretKey = 0;
int g_accountID = 0;

std::vector<GameServer> g_gameServers;
UdpSocket g_gameSocket;