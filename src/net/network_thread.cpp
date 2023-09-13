#include "network_thread.hpp"
#include "../global_data.hpp"
#include "../util.hpp"
#include "tcp_socket.hpp"
#include "udp_socket.hpp"

#include <Geode/utils/web.hpp>
#include <json.hpp>
#include <mutex>
#include <thread>
#include <variant>

namespace log = geode::log;
namespace web = geode::utils::web;

void networkThread() {
    // this thread is exited only in critical conditions, such as winsock not being loaded.
    if (!loadNetLibraries()) {
        log::error("Globed failed to initialize winsock! WSA last error: {}", WSAGetLastError());
        g_errMsgQueue.lock()->push("Globed failed to initialize <cy>WinSock</c>. The mod will <cr>not</c> function. This is likely because your system is low on memory or GD has all networking permissions revoked (sandboxed?). Resolve the issue and restart the game.");
        return;
    }

    if (!g_gameSocket.create()) {
        log::error("Globed failed to initialize a UDP socket! WSA last error: {}", WSAGetLastError());
        g_errMsgQueue.lock()->push("Globed failed to initialize a <cy>UDP socket</c> because of a <cr>network error</c>. The mod will <cr>not</c> function. If you believe this shouldn't be happening, please send the logs to the developer.");
        return;
    }

    auto modVersion = Mod::get()->getVersion().toString();
    modVersion.erase(0, 1); // remove 'v' from 'v1.1.1'

    std::thread recvT(recvThread);
    std::thread keepaliveT(keepaliveThread);

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

                if (g_gameSocket.established) {
                    globed_util::net::disconnect(false, true);
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

                if (!storedServer.empty() && !g_gameSocket.connected) {
                    globed_util::net::connectToServer(storedServer);
                }
            } else if (std::holds_alternative<PingServers>(message)) {
                // if we have GlobedMenuLayer opened then we ping servers every 5 seconds
                pingAllServers();
            } else if (g_gameSocket.established) {
                g_gameSocket.sendMessage(message);
            }
        }

        auto taken = std::chrono::high_resolution_clock::now() - start;
        if (taken < THREAD_SLEEP_DELAY) {
            std::this_thread::sleep_for(THREAD_SLEEP_DELAY - taken);
        }
    }

    // exit

    if (g_gameSocket.established) {
        globed_util::net::disconnect(false, false);
    }
    
    recvT.join();
    keepaliveT.join();

    unloadNetLibraries();
    
    log::info("Main network thread exited. Globed is unloaded!");
}

void testCentralServer(const std::string& modVersion, std::string url) {
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

void pingAllServers() {
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
        
        g_gameSocket.sendPingTo(id, ip, port);
    }
}

void recvThread() {
    while (shouldContinueLooping()) {
        RecvPacket packet;
        try {
            if (!g_gameSocket.poll(500)) {
                continue;
            }

            packet = g_gameSocket.recvPacket();

            if (std::holds_alternative<PacketCheckedIn>(packet)) {
                log::info("Checked in successfully to the game server");
                g_gameSocket.established = true;
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

                globed_util::net::disconnect(true, true);

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
            if (!g_gameSocket.connected) {
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

void keepaliveThread() {
    while (shouldContinueLooping()) {
        if (g_gameSocket.established) {
            g_gameSocket.sendHeartbeat();
            // this fuckery exits the 5 second delay early if g_isModLoaded is set to false
            std::unique_lock lock(g_modLoadedMutex);
            g_modLoadedCv.wait_for(lock, KEEPALIVE_DELAY, [] { return !g_isModLoaded; });
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

    log::info("Keepalive thread exited.");
}

bool shouldContinueLooping() {
    std::lock_guard lock(g_modLoadedMutex);
    return g_isModLoaded;
}