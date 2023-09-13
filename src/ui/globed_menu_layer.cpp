#include "globed_menu_layer.hpp"

constexpr float PING_DELAY = 5.f;
constexpr float REFRESH_DELAY = 0.1f;

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    globed_util::ui::addBackground(this);

    auto menu = CCMenu::create();
    globed_util::ui::addBackButton(this, menu, menu_selector(GlobedMenuLayer::goBack));
    this->addChild(menu);

    globed_util::ui::enableKeyboard(this);

    refreshServers(0.f);

    // error checking
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::checkErrors), this, 0.f, false);
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::pingServers), this, PING_DELAY, false);
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::refreshWeak), this, REFRESH_DELAY, false);

    return true;
}

void GlobedMenuLayer::checkErrors(float dt) {
    globed_util::handleErrors();
}

void GlobedMenuLayer::pingServers(float dt) {
    sendMessage(PingServers {});
}

void GlobedMenuLayer::refreshWeak(float dt) {
    // refreshWeak simply updates the existing GJListLayer, by calling updateValues() on each ServerListCell
    // If it detects that there are servers that should be added/removed, it calls refreshServers(), which completely remakes the list.
    {
        std::lock_guard lock(g_gameServerMutex);
        m_internalServers = g_gameServers;
    }

    auto listCells = m_list->m_listView->m_tableView->m_contentLayer->getChildren();

    unsigned int count = 0;
    if (listCells != nullptr) {
        count = listCells->count();
    }

    unsigned int updated = 0;
    for (unsigned int i = 0; i < count; i++) {
        auto listCell = static_cast<CCNode*>(listCells->objectAtIndex(i));
        auto serverListCell = static_cast<ServerListCell*>(listCell->getChildren()->objectAtIndex(2));
        auto serverId = serverListCell->m_server.id;

        for (const auto& server : m_internalServers) {
            if (server.id == serverId) {
                bool active;
                {
                    std::lock_guard lock(g_gameServerMutex);
                    active = g_gameServerId == serverId;
                }
                
                long long ping = -1;
                int players = 0;

                if (!active) {
                    auto pings = g_gameServersPings.lock();

                    if (pings->contains(serverId)) {
                        auto pingData = pings->at(serverId);
                        ping = pingData.first;
                        players = pingData.second;
                    }
                } else {
                    ping = g_gameServerPing;
                    players = g_gameServerPlayerCount;
                }

                serverListCell->updateValues(players, ping, active);
                updated++;
                break;
            }
        }
    }

    if (updated != m_internalServers.size()) {
        // not enough, new servers were added maybe?
        refreshServers(0.f);
    }
}

void GlobedMenuLayer::refreshServers(float dt) {
    m_internalServers = g_gameServers;

    auto windowSize = CCDirector::get()->getWinSize();

    if (m_list) m_list->removeFromParent();

    auto list = ListView::create(createServerList(), CELL_HEIGHT, LIST_SIZE.width, LIST_SIZE.height);

    m_list = GJListLayer::create(list, "Servers", { 0, 0, 0, 180 }, 358.f, 220.f);  
    m_list->setZOrder(2);
    m_list->setPosition(windowSize / 2 - m_list ->getScaledContentSize() / 2);

    this->addChild(m_list);
}

void GlobedMenuLayer::sendMessage(Message msg) {
    g_netMsgQueue.lock()->push(msg);
}

CCArray* GlobedMenuLayer::createServerList() {
    auto servers = CCArray::create();
    std::lock_guard lock(g_gameServerMutex);
    std::string connectedId = g_gameServerId;

    int activePlayers = g_gameServerPlayerCount;
    long long activePing = g_gameServerPing;

    for (const auto server : m_internalServers) {
        bool active = connectedId == server.id;

        long long ping = -1;
        int players = 0;

        if (active) {
            ping = activePing;
            players = activePlayers;
        } else {
            auto guard = g_gameServersPings.lock();
            for (const auto pingEntry : *guard) {
                if (pingEntry.first == server.id) {
                    ping = pingEntry.second.first;
                    players = pingEntry.second.second;
                    break;
                }

            }
        }

        servers->addObject(ServerListCell::create(
            server,
            ping,
            players,
            {LIST_SIZE.width, CELL_HEIGHT},
            active
        ));
    }

    return servers;
}

DEFAULT_GOBACK_DEF(GlobedMenuLayer)
DEFAULT_KEYDOWN_DEF(GlobedMenuLayer)
DEFAULT_CREATE_DEF(GlobedMenuLayer)