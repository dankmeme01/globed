#include "network_handler.hpp"
#include "tcp_socket.hpp"
#include "udp_socket.hpp"
#include <global_data.hpp>
#include <util.hpp>
#include <smart_message_queue.hpp>

#include <Geode/utils/web.hpp>
#include <json.hpp>
#include <mutex>
#include <thread>
#include <variant>

namespace log = geode::log;

NetworkHandler::NetworkHandler(int secretKey) : gameSocket(0, secretKey) {
    if (!loadNetLibraries()) {
        log::error("Globed failed to initialize winsock! WSA last error: {}", getLastNetError());
        g_errMsgQueue.push("Globed failed to initialize <cy>WinSock</c>. The mod will <cr>not</c> function. This is likely because your system is low on memory or GD has all networking permissions revoked (sandboxed?). Resolve the issue and restart the game.");
        borked = true;
        return;
    }

    if (!gameSocket.create()) {
        log::error("Globed failed to initialize a UDP socket! WSA last error: {}", getLastNetError());
        g_errMsgQueue.push("Globed failed to initialize a <cy>UDP socket</c> because of a <cr>network error</c>. The mod will <cr>not</c> function. If you believe this shouldn't be happening, please send the logs to the developer.");
        borked = true;
    }
}

NetworkHandler::~NetworkHandler() {
    g_isModLoaded = false;
    
    log::debug("NetworkHandler deinit");

    // the following always fails, join() does nothing.
    // i have no idea why, i spent the whole day debugging it,
    // i've tried getting help from discord, i've rewritten so much stuff,
    // but alas. going to just deal with it.
    // have a cookie 🍪 (it is not rendered on my arch machine btw)

    if (mainThread.joinable()) {
        mainThread.join();
    }

    if (recvThread.joinable()) {
        recvThread.join();
    }

    if (keepaliveThread.joinable()) {
        keepaliveThread.join();
    }

    if (gameSocket.established) {
        disconnect(false, false);
    }

    unloadNetLibraries();
}

bool NetworkHandler::connectToServer(const std::string& id) {
    disconnect(false, false);
    
    std::lock_guard<std::mutex> lock(g_gameServerMutex);
    
    auto it = std::find_if(g_gameServers.begin(), g_gameServers.end(), [id](const GameServer& element) {
        return element.id == id;
    });

    if (it != g_gameServers.end()) {
        GameServer server = *it;

        auto [address, port] = globed_util::net::splitAddress(server.address);
        if (!port) {
            port = 41001;
        }

        if (!gameSocket.connect(address, port)) {
            return false;
        }

        g_gameServerId = server.id;
        gameSocket.sendCheckIn();

        Mod::get()->setSavedValue("last-server-id", id);
        return true;
    }

    return false;
}

void NetworkHandler::disconnect(bool quiet, bool save) {
    // quiet - will not send a Disconnect packet
    // save - will not clear the last-server-id value

    if (!quiet && gameSocket.connected) {
        gameSocket.sendDisconnect();
    }

    if (save) {
        Mod::get()->setSavedValue("last-server-id", std::string(""));
    }

    std::lock_guard<std::mutex> lock(g_gameServerMutex);
    
    // we move the ping and player count into g_gameServerPings
    if (gameSocket.established) {
        auto pings = g_gameServersPings.lock();
        (*pings)[g_gameServerId] = std::make_pair(g_gameServerPing.load(), g_gameServerPlayerCount.load());
    }

    gameSocket.disconnect();

    g_gameServerPlayerCount = 0;
    g_gameServerPing = -1;
    g_gameServerTps = 0;
    
    g_gameServerId = "";
}

void NetworkHandler::run() {
    mainThread = std::thread(&NetworkHandler::tMain, this);
    recvThread = std::thread(&NetworkHandler::tRecv, this);
    keepaliveThread = std::thread(&NetworkHandler::tKeepalive, this);

    // SetThreadDescription(mainThread.native_handle(), L"Globed Main thread");
    // SetThreadDescription(recvThread.native_handle(), L"Globed Recv thread");
    // SetThreadDescription(keepaliveThread.native_handle(), L"Globed Keepalive thread");
}

void NetworkHandler::tMain() {
    std::string activeCentralServer = Mod::get()->getSavedValue<std::string>("central");
    globed_util::net::testCentralServer(PROTOCOL_VERSION, activeCentralServer);
    
    while (shouldContinueLooping()) {
        g_netMsgQueue.waitForMessages();
        auto message = g_netMsgQueue.pop();

        if (std::holds_alternative<NMCentralServerChanged>(message)) {
            activeCentralServer = std::get<NMCentralServerChanged>(message).server;

            if (gameSocket.established) {
                disconnect(false, true);
            }

            // clear all existing game servers
            g_gameServersPings.lock()->clear();
            {
                std::lock_guard<std::mutex> lock(g_gameServerMutex);
                g_gameServers.clear();
            }
            
            globed_util::net::testCentralServer(PROTOCOL_VERSION, activeCentralServer);
            pingAllServers();
        } else if (std::holds_alternative<NMMenuLayerEntry>(message)) {
            // when menu layer is finally loaded, try to connect to a saved server
            gameSocket.accountId = GJAccountManager::sharedState()->m_accountID;
            if (gameSocket.accountId == 0) {
                log::info("got GameLoadedData but the account ID is 0, not proceeding");
                disconnect(true, true); // quiet set to true because we lost the account ID
            } else {
                auto storedServer = Mod::get()->getSavedValue<std::string>("last-server-id");

                if (!storedServer.empty() && !gameSocket.connected) {
                    connectToServer(storedServer);
                }
            }

        } else if (std::holds_alternative<NMPingServers>(message)) {
            // if we have GlobedMenuLayer opened then we ping servers every 5 seconds
            pingAllServers();
        } else if (gameSocket.established) {
            gameSocket.sendMessage(message);
        }
    }
        
    log::info("Main network thread exited. Globed is unloaded!");
}

void NetworkHandler::tRecv() {
    while (shouldContinueLooping()) {
        RecvPacket packet;
        try {
            if (!gameSocket.poll(500)) {
                continue;
            }

            packet = gameSocket.recvPacket();

            if (std::holds_alternative<PacketCheckedIn>(packet)) {
                log::info("Checked in successfully to the game server");
                gameSocket.established = true;
                // if we pinged the server prior, move it into g_gameServerPing and g_gameServerPlayerCount
                auto pings = g_gameServersPings.lock();

                if (pings->contains(g_gameServerId)) {
                    auto values = pings->at(g_gameServerId);
                    g_gameServerPing = values.first;
                    g_gameServerPlayerCount = values.second;
                }

                g_gameServerTps = std::get<PacketCheckedIn>(packet).tps;
            } else if (std::holds_alternative<PacketKeepaliveResponse>(packet)) {
                auto pkt = std::get<PacketKeepaliveResponse>(packet);
                g_gameServerPing = pkt.ping;
                g_gameServerPlayerCount = pkt.playerCount;
            } else if (std::holds_alternative<PacketServerDisconnect>(packet)) {
                auto reason = std::get<PacketServerDisconnect>(packet).message;
                log::warn("Server disconnect, reason: {}", reason);

                std::string serverName = "Unknown";
                {
                    std::lock_guard lock(g_gameServerMutex);
                    for (const auto& server : g_gameServers) {
                        if (server.id == g_gameServerId) {
                            serverName = server.name;
                            break;
                        }
                    }
                }

                auto errMessage = fmt::format("You were disconnected from the game server <cg>{}</c>. Message from the server:\n\n <cy>{}</c>", serverName, reason);

                disconnect(true, true);

                g_errMsgQueue.push(errMessage);
            } else if (std::holds_alternative<PacketLevelData>(packet)) {
                auto data = std::get<PacketLevelData>(packet).players;
                g_pCorrector.feedRealData(data);
            } else if (std::holds_alternative<PacketPingResponse>(packet)) {
                auto response = std::get<PacketPingResponse>(packet);
                auto lockguard = g_gameServersPings.lock();
                (*lockguard)[response.serverId] = std::make_pair(response.ping, response.playerCount);
            } else if (std::holds_alternative<PacketAccountDataResponse>(packet)) {
                auto response = std::get<PacketAccountDataResponse>(packet);
                (*g_accDataCache.lock())[response.playerId] = response.data;
            } else if (std::holds_alternative<PacketLevelListResponse>(packet)) {
                auto response = std::get<PacketLevelListResponse>(packet);
                *g_levelsList.lock() = response.levels;
                g_levelsLoading = false;
            }
        } catch (std::exception e) {
            // if an error occured while we are disconnected then it's alright
            if (!gameSocket.connected) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                continue;
            } else {
                log::warn("error in recvThread: {}", e.what());
                g_warnMsgQueue.push(e.what());
            }

        }
    }

    log::info("Data receiving thread exited.");
}

void NetworkHandler::tKeepalive() {
    while (shouldContinueLooping()) {
        if (gameSocket.established) {
            gameSocket.sendHeartbeat();
            std::this_thread::sleep_for(KEEPALIVE_DELAY);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

    log::info("Keepalive thread exited.");
}

void NetworkHandler::pingAllServers() {
    std::unordered_map<std::string, std::pair<std::string, unsigned short>> addresses;
    {
        std::lock_guard lock(g_gameServerMutex);
        for (const auto& server : g_gameServers) {
            if (server.id == g_gameServerId) continue;

            addresses.insert(std::make_pair(server.id, globed_util::net::splitAddress(server.address)));
        }
    }

    log::debug("pinging {} servers", addresses.size());
    for (const auto& address : addresses) {
        const auto& id = address.first;
        const auto& ip = address.second.first;
        const auto& port = address.second.second;
        log::debug("pinging server {}:{}", ip, port);
        
        gameSocket.sendPingTo(id, ip, port);
    }
}

bool NetworkHandler::shouldContinueLooping() {
    return g_isModLoaded;
}


int NetworkHandler::getAccountId() {
    return gameSocket.accountId;
}

int NetworkHandler::getSecretKey() {
    return gameSocket.secretKey;
}

bool NetworkHandler::established() {
    return gameSocket.established;
}