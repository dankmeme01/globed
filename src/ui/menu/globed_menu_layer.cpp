#include "globed_menu_layer.hpp"

#include <ui/popup/central_url_popup.hpp>
#include <ui/levels/globed_levels_layer.hpp>

constexpr float PING_DELAY = 5.f;
constexpr float REFRESH_DELAY = 0.1f;

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;
    auto windowSize = CCDirector::get()->getWinSize();

    globed_util::ui::addBackground(this);

    auto menu = CCMenu::create();
    globed_util::ui::addBackButton(this, menu, menu_selector(GlobedMenuLayer::goBack));
    this->addChild(menu);

    globed_util::ui::enableKeyboard(this);

    // layout for da buttons
    auto buttonsLayout = ColumnLayout::create();
    auto buttonsMenu = CCMenu::create();
    buttonsMenu->setLayout(buttonsLayout);
    buttonsLayout->setAxisAlignment(AxisAlignment::Start);
    buttonsLayout->setGap(5.0f);
    buttonsLayout->setAutoScale(false);

    buttonsMenu->setPosition({15.f, 15.f});
    buttonsMenu->setAnchorPoint({0.f, 0.f});

    this->addChild(buttonsMenu);

    // add a button for hard refresh
    auto hrSprite = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    auto hrButton = CCMenuItemSpriteExtra::create(hrSprite, this, menu_selector(GlobedMenuLayer::onHardRefreshButton));
    buttonsMenu->addChild(hrButton);

    // add a button for server configuration
    auto curlSprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    auto curlButton = CCMenuItemSpriteExtra::create(curlSprite, this, menu_selector(GlobedMenuLayer::onOpenCentralUrlButton));
    buttonsMenu->addChild(curlButton);

    // add a button for viewing levels
    auto levelsSprite = CCSprite::createWithSpriteFrameName("GJ_menuBtn_001.png");
    levelsSprite->setScale(0.8f);
    auto levelsButton = CCMenuItemSpriteExtra::create(levelsSprite, this, menu_selector(GlobedMenuLayer::onOpenLevelsButton));
    buttonsMenu->addChild(levelsButton);

    buttonsMenu->updateLayout();

    refreshServers(0.f);

    // error checking
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::checkErrors), this, 0.f, false);

    // server pinging
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::pingServers), this, PING_DELAY, false);

    // weak refreshing
    CCScheduler::get()->scheduleSelector(schedule_selector(GlobedMenuLayer::refreshWeak), this, REFRESH_DELAY, false);

    pingServers(0.f);

    return true;
}

void GlobedMenuLayer::checkErrors(float dt) {
    globed_util::handleErrors();
}

void GlobedMenuLayer::pingServers(float dt) {
    sendMessage(NMPingServers {});
}

void GlobedMenuLayer::onOpenCentralUrlButton(CCObject* sender) {
    CentralUrlPopup::create()->show();
}

void GlobedMenuLayer::onHardRefreshButton(CCObject* sender) {
    auto central = Mod::get()->getSavedValue<std::string>("central");
    if (!central.ends_with('/')) {
        central += '/';
    }

    globed_util::net::testCentralServer(PROTOCOL_VERSION, central);
    refreshServers(0.f);
}

void GlobedMenuLayer::onOpenLevelsButton(CCObject* sender) {
    auto director = CCDirector::get();
    auto layer = GlobedLevelsLayer::create();
    layer->setID("dankmeme.globed/layer-globed-levels");
    
    auto destScene = globed_util::sceneWithLayer(layer);

    auto transition = CCTransitionFade::create(0.5f, destScene, ccBLACK);
    director->pushScene(transition);
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

        bool active = false;
        if (g_networkHandler->established()) {
            std::lock_guard lock(g_gameServerMutex);
            active = g_gameServerId == serverId;
        }

        for (const auto& server : m_internalServers) {
            if (server.id == serverId) {
                
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

                serverListCell->updateValues(players, ping, active || ping != -1, active);
                updated++;
                break;
            }
        }
    }

    if (updated != count || count != m_internalServers.size()) {
        // hard refresh
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

void GlobedMenuLayer::sendMessage(NetworkThreadMessage msg) {
    g_netMsgQueue.push(msg);
}

CCArray* GlobedMenuLayer::createServerList() {
    auto servers = CCArray::create();
    std::lock_guard lock(g_gameServerMutex);
    std::string connectedId = g_gameServerId;

    int activePlayers = g_gameServerPlayerCount;
    long long activePing = g_gameServerPing;

    for (const auto server : m_internalServers) {
        bool active = connectedId == server.id && g_networkHandler->established();

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
            active || ping != -1,
            active
        ));
    }

    return servers;
}

DEFAULT_GOBACK_DEF(GlobedMenuLayer)
DEFAULT_KEYBACK_DEF(GlobedMenuLayer)
DEFAULT_CREATE_DEF(GlobedMenuLayer)