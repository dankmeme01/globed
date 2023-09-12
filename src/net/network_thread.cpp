#include "network_thread.hpp"
#include "../global_data.hpp"
#include "../util.hpp"
#include "tcp_socket.hpp"
#include "udp_socket.hpp"

#include <Geode/utils/web.hpp>
#include <json.hpp>
#include <mutex>
#include <sstream>
#include <thread>
#include <variant>

namespace log = geode::log;
namespace web = geode::utils::web;

void networkThread() {
    if (!loadNetLibraries()) {
        log::error("Globed failed to initialize winsock!");
        std::lock_guard<std::mutex> lock(g_errMsgMutex);
        g_errMsgQueue.push("Globed failed to initialize WinSock. The mod will not function. This is likely because your system is low on memory or GD has all networking permissions revoked (sandboxed?).");
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

        std::lock_guard<std::mutex> lock(g_errMsgMutex);
        std::string errMessage = fmt::format("Globed failed to make a request to the central server while trying to fetch its version. This is likely because the server is down, or your device is unable to connect to it. If you want to use the mod, resolve the network problem or change the central server URL in settings, and restart the game.\n\nError: <cy>{}</c>", error);
        g_errMsgQueue.push(errMessage);

        return;
    }

    auto serverV = serverVersion.unwrap();
    if (modVersion != serverV) {
        log::error("Server version mismatch: client at {}, server at {}", modVersion, serverV);
        
        unloadNetLibraries();

        std::lock_guard<std::mutex> lock(g_errMsgMutex);
        auto errMessage = fmt::format("Globed mod version either too old or too new. Mod's version is <cy>v{}</c>, while central server's version is <cy>v{}</c>. Resolve the version conflict (usually by updating the mod) and restart the game.", modVersion, serverV);
        g_errMsgQueue.push(errMessage);

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

        std::lock_guard<std::mutex> lock(g_errMsgMutex);
        g_errMsgQueue.push(errMessage);

        return;
    }

    g_gameSocket.create();

    std::thread recvT(recvThread);
    std::thread keepaliveT(keepaliveThread);


    while (g_isModLoaded) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<Message> msgBuf;
        {
            std::lock_guard<std::mutex> lock(g_netMutex);

            while (!g_netMsgQueue.empty()) {
                Message message = g_netMsgQueue.front();
                g_netMsgQueue.pop();

                // when menu layer is finally loaded, try to connect to a saved server
                if (std::holds_alternative<GameLoadedData>(message)) {
                    auto storedServer = Mod::get()->getSavedValue<std::string>("last-server-id");

                    if (!storedServer.empty() && !g_gameSocket.connected) {
                        globed_util::net::connectToServer(storedServer);
                    }
                    break;
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
    
    globed_util::net::disconnect();
    unloadNetLibraries();
}

void recvThread() {
    while (g_isModLoaded) {
        if (!g_gameSocket.connected) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        }

        RecvPacket packet;
        try {
            packet = g_gameSocket.recvPacket();
            if (std::holds_alternative<PacketCheckedIn>(packet)) {
                log::info("checked in successfully to the game server");
            } else if (std::holds_alternative<PacketKeepaliveResponse>(packet)) {
                log::info("got keepalive response");
            } else if (std::holds_alternative<PacketServerDisconnect>(packet)) {
                auto reason = std::get<PacketServerDisconnect>(packet).message;
                log::warn("server disconnected us!");
                log::warn(reason);

                auto errMessage = fmt::format("You were disconnected from the game server. Message from the server:\n\n <cy>{}</c>", g_gameServerId);

                globed_util::net::disconnect();

                std::lock_guard<std::mutex> lock(g_errMsgMutex);
                g_errMsgQueue.push(errMessage);
            } else if (std::holds_alternative<PacketLevelData>(packet)) {
                log::debug("got player data");
                auto data = std::get<PacketLevelData>(packet).players;
                std::lock_guard<std::mutex> lock(g_netRMutex);
                g_netRPlayers = data;
            }
        } catch (std::exception e) {
            log::error(e.what());
            std::lock_guard<std::mutex> lock(g_warnMsgMutex);
            g_warnMsgQueue.push(e.what());
        }
    }
}

void keepaliveThread() {
    while (g_isModLoaded) {
        if (g_gameSocket.connected) {
            g_gameSocket.sendHeartbeat();
        }

        std::this_thread::sleep_for(KEEPALIVE_DELAY);
    }
}