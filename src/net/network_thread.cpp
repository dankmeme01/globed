#include "network_thread.hpp"
#include "../inter_thread_data.hpp"
#include "../util.hpp"
#include "tcp_socket.hpp"
#include "udp_socket.hpp"

namespace log = geode::log;

void networkThread() {
    if (!loadNetLibraries()) {
        log::error("Globed failed to initialize winsock!");
        globed_util::errorPopup("Globed failed to initialize WinSock. The mod will not function. This is likely because your system is low on memory or GD has all networking permissions revoked (sandboxed?).");
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