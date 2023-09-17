#pragma once
#include <Geode/Geode.hpp>
#include "../data/player_account_data.hpp"
#include "../data/player_data.hpp"

using namespace geode::prelude;

const PlayerAccountData DEFAULT_DATA = {
    .cube = 1,
    .ship = 1,
    .ball = 1,
    .ufo = 1,
    .wave = 1,
    .robot = 1,
    .spider = 1,
    .color1 = 0,
    .color2 = 3,
    .name = "Player",
};

bool operator==(const PlayerAccountData& lhs, const PlayerAccountData& rhs);

class RemotePlayer : public CCNode {
    // use game manager colorForIdx to convert int into cccolor3b
    // use icon cache, if doesn't exist then use default icons
    // have some mechanism that will periodically check for icons in the cache if none exist
    // in play layer somehow send messages about icon requests, figure out how
    // gl
public:
    bool init(PlayerAccountData icons, bool isSecond_);

    void tick(IconGameMode mode);
    void setActiveIcon(IconGameMode mode);
    void updateData(PlayerAccountData data, bool areDefaults = false);

    // those are proxy to innerNode.setXXX();
    // needed so that the player name label does not rotate when used in PPA engines
    void setRotationX(float x);
    void setRotationY(float y);
    void setRotation(float y);
    void setScale(float scale);
    void setScaleX(float scale);
    void setScaleY(float scale);
    float getRotationX();
    float getRotationY();
    float getRotation();

    static RemotePlayer* create(bool isSecond, PlayerAccountData data = DEFAULT_DATA);

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
    CCLabelBMFont* labelName = nullptr;
    CCNode* innerNode;
    
    std::string name;
    bool isSecond;
};