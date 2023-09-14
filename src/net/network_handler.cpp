#include "network_handler.hpp"
#include "../global_data.hpp"
#include "../util.hpp"
#include "Geode/loader/Log.hpp"
#include "tcp_socket.hpp"
#include "udp_socket.hpp"

#include <Geode/utils/web.hpp>
#include <json.hpp>
#include <mutex>
#include <thread>
#include <variant>

namespace log = geode::log;
namespace web = geode::utils::web;

NetworkHandler::NetworkHandler() : gameSocket(0, 0) {
    if (!loadNetLibraries()) {
        log::error("Globed failed to initialize winsock! WSA last error: {}", WSAGetLastError());
        g_errMsgQueue.lock()->push("Globed failed to initialize <cy>WinSock</c>. The mod will <cr>not</c> function. This is likely because your system is low on memory or GD has all networking permissions revoked (sandboxed?). Resolve the issue and restart the game.");
        borked = true;
        return;
    }

    if (!gameSocket.create()) {
        log::error("Globed failed to initialize a UDP socket! WSA last error: {}", WSAGetLastError());
        g_errMsgQueue.lock()->push("Globed failed to initialize a <cy>UDP socket</c> because of a <cr>network error</c>. The mod will <cr>not</c> function. If you believe this shouldn't be happening, please send the logs to the developer.");
        borked = true;
    }
}

NetworkHandler::~NetworkHandler() {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    g_isModLoaded = false;
    // g_modLoadedCv.notify_all();
    cvarMain.notify_one();
    cvarKeepalive.notify_one();
    
    log::debug("NetworkHandler deinit");

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

    if (!gameSocket.established) {
        return;
    }

    if (!quiet) {
        gameSocket.sendDisconnect();
    }

    if (save) {
        Mod::get()->setSavedValue("last-server-id", std::string(""));
    }

    gameSocket.disconnect();
    g_gameServerPlayerCount = 0;
    g_gameServerPing = -1;
    
    std::lock_guard<std::mutex> lock(g_gameServerMutex);
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
    auto modVersion = Mod::get()->getVersion().toString();
    modVersion.erase(0, 1); // remove 'v' from 'v1.1.1'

    std::string activeCentralServer = Mod::get()->getSavedValue<std::string>("central");
    testCentralServer(modVersion, activeCentralServer);
    
    while (shouldContinueLooping()) {
        auto start = std::chrono::high_resolution_clock::now();

        // pop all messages into a temp vector to avoid holding the mutex for long
        std::vector<Message> msgBuf;
        {
            auto queue = g_netMsgQueue.lock();

            while (!queue->empty()) {
                Message message = queue->front();
                queue->pop();
                msgBuf.push_back(message);
            }
        }

        for (const auto& message : msgBuf) {
            if (std::holds_alternative<CentralServerChanged>(message)) {
                activeCentralServer = std::get<CentralServerChanged>(message).server;

                if (gameSocket.established) {
                    disconnect(false, true);
                }

                // clear all existing game servers
                {
                    g_gameServersPings.lock()->clear();
                    std::lock_guard<std::mutex> lock(g_gameServerMutex);
                    g_gameServers.clear();
                }
                
                testCentralServer(modVersion, activeCentralServer);
            } else if (std::holds_alternative<GameLoadedData>(message)) {
                // when menu layer is finally loaded, try to connect to a saved server
                auto storedServer = Mod::get()->getSavedValue<std::string>("last-server-id");

                if (!storedServer.empty() && !gameSocket.connected) {
                    connectToServer(storedServer);
                }
            } else if (std::holds_alternative<PingServers>(message)) {
                // if we have GlobedMenuLayer opened then we ping servers every 5 seconds
                pingAllServers();
            } else if (gameSocket.established) {
                gameSocket.sendMessage(message);
            }
        }

        auto taken = std::chrono::high_resolution_clock::now() - start;
        if (taken < THREAD_SLEEP_DELAY) {
            // this fuckery exits the 5 second delay early if g_isModLoaded is set to false
            std::unique_lock lock(modLoadedMtxMain);
            cvarMain.wait_for(lock, THREAD_SLEEP_DELAY - taken, [] { return !g_isModLoaded; });
        }
    }

        
    log::info("Main network thread exited. Globed is unloaded!");
}

void NetworkHandler::testCentralServer(const std::string& modVersion, std::string url) {
    if (url.empty()) {
        log::info("Central server was set to an empty string, disabling.");
        return;
    }

    log::info("Trying to switch to central server {}", url);

    if (!url.ends_with('/')) {
        url += '/';
    }

    auto versionURL = url + "version";
    auto serversURL = url + "servers";

    auto serverVersionRes = web::fetch(versionURL);

    if (serverVersionRes.isErr()) {
        auto error = serverVersionRes.unwrapErr();
        log::warn("failed to fetch server version: {}: {}", versionURL, error);

        std::string errMessage = fmt::format("Globed failed to reach the central server endpoint <cy>{}</c>, likely because the server is down, your internet is down, or an invalid URL was entered.\n\nError: <cy>{}</c>", versionURL, error);
        g_errMsgQueue.lock()->push(errMessage);

        return;
    }

    auto serverVersion = serverVersionRes.unwrap();
    if (modVersion != serverVersion) {
        log::warn("Server version mismatch: client at {}, server at {}", modVersion, serverVersion);

        auto errMessage = fmt::format("Globed mod version is incompatible with the central server. Mod's version is <cy>v{}</c>, while central server's version is <cy>v{}</c>. To use this server, update the outdated client/server and try again.", modVersion, serverVersion);
        g_errMsgQueue.lock()->push(errMessage);

        return;
    }

    try {
        globed_util::net::updateGameServers(serversURL);
    } catch (std::exception e) {
        log::warn("updateGameServers failed: {}", e.what());
        auto errMessage = fmt::format("Globed failed to parse server list sent by the central server. This is likely due to a misconfiguration on the server.\n\nError: <cy>{}</c>", e.what());

        g_errMsgQueue.lock()->push(errMessage);
    }

    log::info("Successfully updated game servers from the central server");
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

    for (const auto& address : addresses) {
        const auto& id = address.first;
        const auto& ip = address.second.first;
        const auto& port = address.second.second;
        
        gameSocket.sendPingTo(id, ip, port);
    }
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

                g_errMsgQueue.lock()->push(errMessage);
            } else if (std::holds_alternative<PacketLevelData>(packet)) {
                auto data = std::get<PacketLevelData>(packet).players;
                g_netRPlayers.lock() = data;
            } else if (std::holds_alternative<PacketPingResponse>(packet)) {
                auto response = std::get<PacketPingResponse>(packet);
                auto lockguard = g_gameServersPings.lock();
                (*lockguard)[response.serverId] = std::make_pair(response.ping, response.playerCount);
            }
        } catch (std::exception e) {
            // if an error occured while we are disconnected then it's alright
            if (!gameSocket.connected) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                continue;
            } else {
                log::warn("error in recvThread: {}", e.what());
                // g_warnMsgQueue.lock()->push(e.what());
            }

        }
    }

    log::info("Data receiving thread exited.");
}

void NetworkHandler::tKeepalive() {
    log::debug("kl: enter");
    while (shouldContinueLooping()) {
        log::debug("kl: loop1");
        if (gameSocket.established) {
            log::debug("kl: loop e1");
            gameSocket.sendHeartbeat();
            log::debug("kl: loop e2");
            // this fuckery exits the 5 second delay early if g_isModLoaded is set to false
            std::unique_lock lock(modLoadedMtxKeepalive);
            log::debug("kl: loop e3");
            cvarKeepalive.wait_for(lock, KEEPALIVE_DELAY, [] { return !g_isModLoaded; });
            log::debug("kl: loop e4");
        } else {
            log::debug("kl: loop l1");
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            log::debug("kl: loop l2");
        }
    }

    log::info("Keepalive thread exited.");
}

bool NetworkHandler::shouldContinueLooping() {
    return g_isModLoaded;
}