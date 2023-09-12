#include "globed_menu_layer.hpp"

constexpr std::chrono::seconds PING_DELAY(5);
constexpr std::chrono::seconds REFRESH_DELAY(1);

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    globed_util::ui::addBackground(this);

    auto menu = CCMenu::create();

    globed_util::ui::addBackButton(this, menu, menu_selector(GlobedMenuLayer::goBack));
    globed_util::ui::enableKeyboard(this);

    refreshServers();

    this->addChild(menu);

    // error checking
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::checkErrorsAndPing), this, 0.0f, false);
    
    m_lastPing = std::chrono::system_clock::now();
    m_lastRefresh = std::chrono::system_clock::now();

    return true;
}

void GlobedMenuLayer::checkErrorsAndPing(float unused) {
    globed_util::handleErrors();
    auto now = std::chrono::system_clock::now();
    if (now - m_lastPing > PING_DELAY) {
        m_lastPing = now;
        sendMessage(PingServers {});
    }

    if (now - m_lastRefresh > REFRESH_DELAY) {
        m_lastRefresh = now;
        refreshServers();
    }
}

void GlobedMenuLayer::refreshServers() {
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
    std::string connectedId;
    {
        std::lock_guard lock(g_gameServerMutex);
        connectedId = g_gameServerId;
    }

    std::lock_guard lock(g_gameServerMutex);
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
            for (const auto pingEntry : g_gameServersPings) {
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
            active,
            this,
            &GlobedMenuLayer::refreshServers
        ));
    }

    return servers;
}

DEFAULT_GOBACK_DEF(GlobedMenuLayer)
DEFAULT_KEYDOWN_DEF(GlobedMenuLayer)
DEFAULT_CREATE_DEF(GlobedMenuLayer)