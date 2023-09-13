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
    if (!loadNetLibraries()) {
        log::error("Globed failed to initialize winsock!");
        g_errMsgQueue.lock()->push("Globed failed to initialize WinSock. The mod will not function. This is likely because your system is low on memory or GD has all networking permissions revoked (sandboxed?).");
        return;
    }

    auto modVersion = Mod::get()->getVersion().toString();
    modVersion.erase(0, 1); // remove 'v' from 'v1.1.1'

    // auto centralURL = Mod::get()->getSettingValue<std::string>("central");
    auto centralURL = std::string("http://127.0.0.1:41000/");

    if (centralURL.empty()) {
        log::warn("Central URL not set, aborting");
        unloadNetLibraries();
        return;
    }

    auto versionURL = centralURL + (centralURL.ends_with('/') ? "version" : "/version");
    auto serverVersion = web::fetch(versionURL);

    if (serverVersion.isErr()) {
        auto error = serverVersion.unwrapErr();
        log::error("failed to fetch server version: {}: {}", versionURL, error);

        unloadNetLibraries();

        std::string errMessage = fmt::format("Globed failed to make a request to the central server while trying to fetch its version. This is likely because the server is down, or your device is unable to connect to it. If you want to use the mod, resolve the network problem or change the central server URL in settings, and restart the game.\n\nError: <cy>{}</c>", error);
        g_errMsgQueue.lock()->push(errMessage);

        return;
    }

    auto serverV = serverVersion.unwrap();
    if (modVersion != serverV) {
        log::error("Server version mismatch: client at {}, server at {}", modVersion, serverV);
        
        unloadNetLibraries();

        auto errMessage = fmt::format("Globed mod version either too old or too new. Mod's version is <cy>v{}</c>, while central server's version is <cy>v{}</c>. Resolve the version conflict (usually by updating the mod) and restart the game.", modVersion, serverV);
        g_errMsgQueue.lock()->push(errMessage);

        return;
    }

    auto serversURL = centralURL + (centralURL.ends_with('/') ? "servers" : "/servers");

    try {
        if (!globed_util::net::updateGameServers(serversURL)) {
            return;
        }
    } catch (std::exception e) {
        log::error(e.what());
        auto errMessage = fmt::format("Globed failed to parse server list sent by the central server. This is likely due to a misconfiguration on the server.\n\nError: <cy>{}</c>", e.what());

        unloadNetLibraries();

        g_errMsgQueue.lock()->push(errMessage);

        return;
    }

    if (!g_gameSocket.create()) {
        log::error("Globed failed to initialize a UDP socket!");
        g_errMsgQueue.lock()->push("Globed failed to initialize a UDP socket because of a network error. The mod will not function.");
        return;
    }

    std::thread recvT(recvThread);
    std::thread keepaliveT(keepaliveThread);

    while (shouldContinueLooping()) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<Message> msgBuf;
        {
            auto queue = g_netMsgQueue.lock();

            while (!queue->empty()) {
                Message message = queue->front();
                queue->pop();

                // when menu layer is finally loaded, try to connect to a saved server
                if (std::holds_alternative<GameLoadedData>(message)) {
                    auto storedServer = Mod::get()->getSavedValue<std::string>("last-server-id");

                    if (!storedServer.empty() && !g_gameSocket.connected) {
                        globed_util::net::connectToServer(storedServer);
                    }
                    break;
                }

                // if we have GlobedMenuLayer opened then we ping servers every 5 seconds
                if (std::holds_alternative<PingServers>(message)) {
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

                    continue;
                }
                msgBuf.push_back(message);
            }
        }

        if (!g_gameSocket.connected) {
            std::this_thread::sleep_for(std::chrono::duration<double>(0.5f));
            continue;
        }

        for (const auto& message : msgBuf) {
            g_gameSocket.sendMessage(message);
        }

        auto taken = std::chrono::high_resolution_clock::now() - start;
        if (taken < THREAD_SLEEP_DELAY) {
            std::this_thread::sleep_for(THREAD_SLEEP_DELAY - taken);
        }
    }

    globed_util::net::disconnect(false, false);
    unloadNetLibraries();

    recvT.join();
    keepaliveT.join();

    log::info("Main network thread exited. Globed is unloaded!");
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
                log::info("checked in successfully to the game server");
                g_gameSocket.established = true;
            } else if (std::holds_alternative<PacketKeepaliveResponse>(packet)) {
                auto pkt = std::get<PacketKeepaliveResponse>(packet);
                std::lock_guard lock(g_gameServerMutex);
                g_gameServerPing = pkt.ping;
                g_gameServerPlayerCount = pkt.playerCount;
            } else if (std::holds_alternative<PacketServerDisconnect>(packet)) {
                auto reason = std::get<PacketServerDisconnect>(packet).message;
                log::warn("server disconnected us!");
                log::warn(reason);

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
                std::lock_guard lock(g_gameServerMutex);
                g_gameServersPings[response.serverId] = std::make_pair(response.ping, response.playerCount);
            }
        } catch (std::exception e) {
            log::error("error in recvThread");
            log::error(e.what());

            // if an error occured while we are disconnected then it's alright
            if (!g_gameSocket.connected) {
                log::warn("error above shall be ignored.");
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                continue;
            }

            g_warnMsgQueue.lock()->push(e.what());
        }
    }

    log::info("Data receiving thread exited.");
}

void keepaliveThread() {
    while (shouldContinueLooping()) {
        if (g_gameSocket.established) {
            g_gameSocket.sendHeartbeat();
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