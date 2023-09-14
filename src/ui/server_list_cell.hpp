#pragma once
#include <Geode/Geode.hpp>
#include "../global_data.hpp"
#include "../util.hpp"

class GlobedMenuLayer;

using namespace geode::prelude;
const ccColor3B ACTIVE_COLOR = ccColor3B{0, 255, 25};
const ccColor3B INACTIVE_COLOR = ccColor3B{255, 255, 255};
// const ccColor3B OFFLINE_COLOR = ccColor3B{64, 64, 64};

class ServerListCell : public CCLayer {
protected:
    CCMenu* m_menu;
    CCLabelBMFont* m_serverName, *m_serverRegion, *m_serverPing;
    CCMenuItemSpriteExtra* m_connectBtn = nullptr;
    std::string m_regionStr, m_pingStr;
    bool m_active = false;
    bool m_online = false;

    bool init(GameServer server, long long ping, int players, const CCSize& size, bool online, bool active) {
        if (!CCLayer::init()) return false;
        m_server = server;
        m_active = active;
        m_online = online;

        m_menu = CCMenu::create();
        m_menu->setPosition(size.width - 40.f, size.height / 2);
        this->addChild(m_menu);

        // name & ping layout

        auto namePingLayer = CCLayer::create();
        auto layout = RowLayout::create();
        layout->setAxisAlignment(AxisAlignment::Start);
        layout->setAutoScale(false);
        namePingLayer->setLayout(layout);
        namePingLayer->setAnchorPoint({0.f, .5f});
        namePingLayer->setPosition({10, size.height / 3 * 2});

        this->addChild(namePingLayer);

        // name

        m_serverName = CCLabelBMFont::create(server.name.c_str(), "bigFont.fnt");
        m_serverName->setAnchorPoint({0.f, 0.f});
        m_serverName->setScale(0.70f);
        namePingLayer->addChild(m_serverName);

        // ping

        m_serverPing = CCLabelBMFont::create("", "goldFont.fnt");
        m_serverPing->setAnchorPoint({0.f, 0.5f});
        m_serverPing->setScale(0.4f);

        namePingLayer->addChild(m_serverPing);
        namePingLayer->updateLayout();

        // region

        m_serverRegion = CCLabelBMFont::create("", "bigFont.fnt");
        m_serverRegion->setAnchorPoint({0.f, .5f});
        m_serverRegion->setPosition({10, size.height * 0.25f});
        m_serverRegion->setScale(0.25f);

        this->addChild(m_serverRegion);

        // // cached buttons
        // m_btnsprJoin = ButtonSprite::create("Join", "bigFont.fnt", "GJ_button_01.png", .8f);
        // m_btnsprLeave = ButtonSprite::create("Leave", "bigFont.fnt", "GJ_button_03.png", .8f);

        updateValues(players, ping, online, active);

        return true;
    }

    void onConnect(CCObject* sender) {
        if (m_active) {
            g_networkHandler->disconnect();
        } else {
            g_networkHandler->connectToServer(m_server.id);
        }
    }

public:
    GameServer m_server;

    void updateValues(int players, long long ping, bool online, bool active) {
        // server name color
        m_serverName->setColor(active ? ACTIVE_COLOR : INACTIVE_COLOR);
        m_serverName->setOpacity(online ? 255 : 128);

        // ping
        m_pingStr = fmt::format("{} ms", ping);
        m_serverPing->setString(m_pingStr.c_str());

        // region & player count
        m_regionStr = fmt::format("Region: {}, players: {}", m_server.region, players);
        m_serverRegion->setString(m_regionStr.c_str());
        m_serverRegion->setColor(active ? ACTIVE_COLOR : INACTIVE_COLOR);
        m_serverRegion->setOpacity(online ? 255 : 128);

        // da button
        if (active != m_active || online != m_online || !m_connectBtn) {
            if (m_connectBtn) {
                m_connectBtn->removeFromParent();
            }

            m_active = active;
            m_online = online;

            if (online) {
                auto btn = ButtonSprite::create(active ? "Leave" : "Join", "bigFont.fnt", active ? "GJ_button_03.png" : "GJ_button_01.png", .8f);
                m_connectBtn = CCMenuItemSpriteExtra::create(btn, this, menu_selector(ServerListCell::onConnect));
                m_connectBtn->setAnchorPoint({1.0f, 0.5f});
                m_connectBtn->setPosition({34, 0});
                m_menu->addChild(m_connectBtn);
            }
        }
    }
    
    static ServerListCell* create(GameServer server, long long ping, int players, const CCSize& size, bool online, bool active) {
        auto ret = new ServerListCell();
        if (ret && ret->init(server, ping, players, size, online, active)) {
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};