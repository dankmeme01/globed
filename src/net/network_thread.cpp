#include "network_thread.hpp"
#include "../global_data.hpp"
#include "../util.hpp"
#include "tcp_socket.hpp"
#include "udp_socket.hpp"

#include <Geode/utils/web.hpp>
#include <json.hpp>
#include <mutex>
#include <sstream>

namespace log = geode::log;
namespace web = geode::utils::web;

void networkThread() {
    if (loadNetLibraries()) {
        log::error("Globed failed to initialize winsock!");
        std::lock_guard<std::mutex> lock(g_errMsgMutex);
        g_errMsgQueue.push("Globed failed to initialize WinSock. The mod will not function. This is likely because your system is low on memory or GD has all networking permissions revoked (sandboxed?).");
        return;
    }

    auto modVersion = Mod::get()->getVersion().toString();
    // auto centralURL = Mod::get()->getSettingValue<std::string>("central");
    auto centralURL = std::string("http://127.0.0.1:41000/");

    if (centralURL.empty()) {
        log::warn("Central URL not set, aborting");
        return;
    }

    auto versionURL = centralURL + (centralURL.ends_with('/') ? "version" : "/version");
    auto serverVersion = web::fetch(versionURL);

    if (serverVersion.isErr()) {
        auto error = serverVersion.unwrapErr();
        log::error("failed to fetch server version: {}: {}", versionURL, error);
        std::lock_guard<std::mutex> lock(g_warnMsgMutex);
        std::stringstream ss;
        ss << "Globed failed to fetch server version: ";
        ss << error;
        g_warnMsgQueue.push(error);
        return;
    }

    auto serverV = serverVersion.unwrap();
    if (modVersion != serverV) {
        log::error("Server version mismatch: client at {}, server at {}", modVersion, serverV);
        
        auto errMessage = fmt::format("Globed mod version either too old or too new. Mod's version is {}, while server's version is {}", modVersion, serverV);

        std::lock_guard<std::mutex> lock(g_errMsgMutex);
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
        auto errMessage = fmt::format("Globed failed to parse server list because of the following exception: {}", e.what());

        std::lock_guard<std::mutex> lock(g_errMsgMutex);
        g_errMsgQueue.push(errMessage);
        return;
    }

    g_gameSocket.create();

    auto storedServer = Mod::get()->getSavedValue<std::string>("last-server-id");

    if (!storedServer.empty()) {
        globed_util::net::connectToServer(storedServer);
    }

    while (g_isModLoaded) {
        std::unique_lock<std::mutex> lock(g_netMutex);
        g_netCVar.wait(lock, [] { return !g_netMsgQueue.empty() || !g_isModLoaded; });

        if (!g_isModLoaded) {
            log::debug("Exiting network thread.");
            break;
        }

        Message message = g_netMsgQueue.front();
        g_netMsgQueue.pop();

        lock.unlock();

        if (std::holds_alternative<PlayerEnterLevelData>(message)) {
            auto data = std::get<PlayerEnterLevelData>(message);
            log::debug("Got level entry, id: {}", data.levelID);
        } else if (std::holds_alternative<PlayerData>(message)) {
            auto data = std::get<PlayerData>(message);
            log::debug("Player data: pos: {},{}, practice: {}, flipped: {}, hidden: {}, dash: {}", data.x, data.y, data.isPractice, data.isUpsideDown, data.isHidden, data.isDashing);
        } else if (std::holds_alternative<PlayerLeaveLevelData>(message)) {
            log::debug("Player left the level");
        } else if (std::holds_alternative<PlayerDeadData>(message)) {
            log::debug("Player died (xD)");
        }
    }
    
    unloadNetLibraries();
}