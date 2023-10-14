#pragma once
#include <Geode/Geode.hpp>
#include <chrono>
#include <map>
#include <vector>

#include <util.hpp>
#include "server_list_cell.hpp"

using namespace geode::prelude;
constexpr float CELL_HEIGHT = 45.0f;
const CCSize LIST_SIZE = { 358.f, 190.f };

class GlobedMenuLayer : public CCLayer {
protected:
    GJListLayer* m_list = nullptr;
    std::vector<GameServer> m_internalServers;
    CCMenuItemSpriteExtra* m_serverLevelsBtn = nullptr;

    bool init();
    void sendMessage(NetworkThreadMessage msg);

    void refreshServers(float dt);
    void refreshWeak(float dt);
    void checkErrors(float dt);
    void pingServers(float dt);

    CCArray* createServerList();
    void onOpenCentralUrlButton(CCObject* sender);
    void onHardRefreshButton(CCObject* sender);
    void onOpenLevelsButton(CCObject* sender);

public:
    DEFAULT_GOBACK
    DEFAULT_KEYBACK
    DEFAULT_CREATE(GlobedMenuLayer)
};