#include "inter_thread_data.hpp"

std::queue<Message> g_netMsgQueue;
std::mutex g_netMutex;
std::condition_variable g_netCVar;

std::atomic_bool g_playerIsPractice = false;

std::atomic_bool g_isModLoaded = true;