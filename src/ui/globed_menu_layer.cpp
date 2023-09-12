#include "globed_menu_layer.hpp"

bool GlobedMenuLayer::init() {
    if (!CCLayer::init()) return false;

    globed_util::ui::addBackground(this);

    auto menu = CCMenu::create();

    globed_util::ui::addBackButton(this, menu, menu_selector(GlobedMenuLayer::goBack));
    globed_util::ui::enableKeyboard(this);

    refreshServers();

    this->addChild(menu);

    this->addChild(m_list);

    return true;
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

CCArray* GlobedMenuLayer::createServerList() {
    auto servers = CCArray::create();
    std::string connectedId;
    {
        std::lock_guard<std::mutex> lock(g_gameServerMutex);
        connectedId = g_gameServerId;
    }

    for (const auto server : m_internalServers) {
        servers->addObject(ServerListCell::create(server, {LIST_SIZE.width, CELL_HEIGHT}, connectedId == server.id, this, &GlobedMenuLayer::refreshServers));
    }

    return servers;
}

DEFAULT_GOBACK_DEF(GlobedMenuLayer)
DEFAULT_KEYDOWN_DEF(GlobedMenuLayer)
DEFAULT_CREATE_DEF(GlobedMenuLayer)