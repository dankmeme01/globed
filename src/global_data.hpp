/*
* Did somebody say global variables are bad? :3
*/

#pragma once

#include <atomic>
#include <unordered_map>
#include <variant>
#include <queue>
#include <mutex>
#include <vector>

#include <correction/corrector.hpp>
#include <net/network_handler.hpp>
#include <net/udp_socket.hpp>
#include <data/data.hpp>
#include <wrapping_mutex.hpp>
#include <smart_message_queue.hpp>

extern bool g_debug;

// to send between any thread -> network thread

extern SmartMessageQueue<NetworkThreadMessage> g_netMsgQueue;

// network thread -> playlayer

extern PlayerCorrector g_pCorrector;

// general lifecycle

extern std::atomic_bool g_isModLoaded;

extern std::atomic_bool g_shownAccountWarning;

extern WrappingMutex<std::string> g_centralURL;

extern WrappingMutex<PlayerAccountData> g_accountData;

// sending errors or warnings to ui thread

extern SmartMessageQueue<std::string> g_errMsgQueue;
extern SmartMessageQueue<std::string> g_warnMsgQueue;

// game servers

extern std::atomic_llong g_gameServerPing;
extern std::atomic_int g_gameServerPlayerCount;
extern std::atomic_ushort g_gameServerTps;
extern std::atomic_llong g_gameServerLastHeartbeat;

extern std::mutex g_gameServerMutex;
extern std::vector<GameServer> g_gameServers;
extern std::string g_gameServerId;

extern WrappingMutex<std::unordered_map<std::string, std::pair<long long, int>>> g_gameServersPings;

// level list

extern std::atomic_bool g_levelsLoading;
extern WrappingMutex<std::unordered_map<int, unsigned short>> g_levelsList;

// level cache

extern WrappingMutex<std::unordered_map<int, GJGameLevel*>> g_levelDataCache;

// player icon cache

extern WrappingMutex<std::unordered_map<int, PlayerAccountData>> g_accDataCache;

// the spectated player, playlayer -> spectate user cell
extern std::atomic_int g_spectatedPlayer;

// current level id, for hiding the players button on PauseLayer
extern std::atomic_int g_currentLevelId;

//Text messages
extern SmartMessageQueue<TextMessage> g_messages;

// silly popup asking you if you want chat or no
extern std::atomic_bool g_hasBeenToMenu;
extern std::atomic_bool g_queueChatPopupInMenu;

extern std::shared_ptr<NetworkHandler> g_networkHandler; // this should be destructed first