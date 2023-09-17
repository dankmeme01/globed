#pragma once
#include <Geode/Geode.hpp>
#include "../data/player_icons.hpp"
#include "../data/player_data.hpp"

using namespace geode::prelude;

constexpr PlayerIconsData DEFAULT_ICONS = {
    .cube = 1,
    .ship = 1,
    .ball = 1,
    .ufo = 1,
    .wave = 1,
    .robot = 1,
    .spider = 1,
    .color1 = 0,
    .color2 = 3,
};

class RemotePlayer : public CCNode {
    // use game manager colorForIdx to convert int into cccolor3b
    // use icon cache, if doesn't exist then use default icons
    // have some mechanism that will periodically check for icons in the cache if none exist
    // in play layer somehow send messages about icon requests, figure out how
    // gl
public:
    bool init(PlayerIconsData icons);

    void tick(IconGameMode mode);
    void setActiveIcon(IconGameMode mode);
    void updateIcons(PlayerIconsData icons);

    static RemotePlayer* create(PlayerIconsData icons = DEFAULT_ICONS);

    bool isDefault;
protected:
    void setValuesAndAdd(ccColor3B primary, ccColor3B secondary);

    IconGameMode lastMode = IconGameMode::NONE;

    SimplePlayer* spCube;
    SimplePlayer* spShip;
    SimplePlayer* spBall;
    SimplePlayer* spUfo;
    SimplePlayer* spWave;
    SimplePlayer* spRobot;
    SimplePlayer* spSpider;
};