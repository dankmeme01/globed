#pragma once
#include <Geode/Geode.hpp>
#include "../global_data.hpp"
#include "../util.hpp"

class GlobedMenuLayer;

using namespace geode::prelude;
const ccColor3B ACTIVE_COLOR = ccColor3B{0, 255, 25};
const ccColor3B INACTIVE_COLOR = ccColor3B{255, 255, 255};

class ServerListCell : public CCLayer {
protected:
    GameServer m_server;
    CCMenu* m_menu;
    std::string m_regionStr;
    bool m_active;
    void (GlobedMenuLayer::*m_refreshFun)();
    GlobedMenuLayer* m_parent;

    bool init(GameServer server, const CCSize& size, bool active, GlobedMenuLayer* parent, void(GlobedMenuLayer::*refreshFun)()) {
        if (!CCLayer::init()) return false;
        m_server = server;
        m_active = active;
        m_refreshFun = refreshFun;
        m_parent = parent;

        m_menu = CCMenu::create();
        m_menu->setPosition(size.width - 40.f, size.height / 2);
        this->addChild(m_menu);

        auto serverName = CCLabelBMFont::create(server.name.c_str(), "bigFont.fnt");
        serverName->setAnchorPoint({0.f, .5f});
        serverName->setPosition({10, size.height / 3 * 2});
        serverName->setScale(0.75f);
        serverName->setColor(active ? ACTIVE_COLOR : INACTIVE_COLOR);

        this->addChild(serverName);

        m_regionStr = std::string("Region: ") + server.region;

        auto serverRegion = CCLabelBMFont::create(m_regionStr.c_str(), "bigFont.fnt");
        serverRegion->setAnchorPoint({0.f, .5f});
        serverRegion->setPosition({10, size.height * 0.25f});
        serverRegion->setScale(0.4f);
        serverRegion->setColor(active ? ACTIVE_COLOR : INACTIVE_COLOR);

        this->addChild(serverRegion);

        // connect button

        auto btnSprite = ButtonSprite::create(active ? "Leave" : "Join", "bigFont.fnt", active ? "GJ_button_03.png" : "GJ_button_01.png", .8f);
        auto connectBtn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(ServerListCell::onConnect));
        
        connectBtn->setAnchorPoint({1.0f, 0.5f});
        connectBtn->setPosition({34, 0});
        m_menu->addChild(connectBtn);

        return true;
    }

    void onConnect(CCObject* sender) {
        if (m_active) {
            globed_util::net::disconnect();
        } else {
            globed_util::net::connectToServer(m_server.id);
        }
        (*m_parent.*m_refreshFun)();
    }

public:
    static ServerListCell* create(GameServer server, const CCSize& size, bool active, GlobedMenuLayer* parent, void(GlobedMenuLayer::*refreshFun)()) {
        auto ret = new ServerListCell();
        if (ret && ret->init(server, size, active, parent, refreshFun)) {
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};