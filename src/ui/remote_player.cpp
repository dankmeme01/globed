#include "remote_player.hpp"
#include "../util.hpp"

bool operator==(const PlayerAccountData& lhs, const PlayerAccountData& rhs) {
    return lhs.cube == rhs.cube \
        && lhs.ship == rhs.ship \
        && lhs.ball == rhs.ball \
        && lhs.ufo == rhs.ufo \
        && lhs.wave == rhs.wave \
        && lhs.robot == rhs.robot \
        && lhs.spider == rhs.spider \
        && lhs.color1 == rhs.color1 \
        && lhs.color2 == rhs.color2;
}

bool RemotePlayer::init(PlayerAccountData data, bool isSecond_) {
    if (!CCNode::init()) {
        return false;
    }

    isSecond = isSecond_;

    innerNode = CCNode::create();
    innerNode->setAnchorPoint({0.5f, 0.5f});
    this->addChild(innerNode);

    isDefault = data == DEFAULT_DATA;

    updateData(data, isDefault);

    return true;
}

void RemotePlayer::tick(IconGameMode mode) {
    if (mode != lastMode) {
        lastMode = mode;
        setActiveIcon(mode);
    }
}

void RemotePlayer::setActiveIcon(IconGameMode mode) {
    SimplePlayer* requiredPlayer;
    switch (mode) {
    case IconGameMode::CUBE:
        requiredPlayer = spCube;
        break;
    case IconGameMode::SHIP:
        requiredPlayer = spShip;
        break;
    case IconGameMode::BALL:
        requiredPlayer = spBall;
        break;
    case IconGameMode::UFO:
        requiredPlayer = spUfo;
        break;
    case IconGameMode::WAVE:
        requiredPlayer = spWave;
        break;
    case IconGameMode::ROBOT:
        requiredPlayer = spRobot;
        break;
    case IconGameMode::SPIDER:
        requiredPlayer = spSpider;
        break;
    default:
        throw std::invalid_argument("Did you really try to set active mode to IconGameMode::NONE?");
    }
    
    // hide all
    for (SimplePlayer* obj : {spCube, spShip, spBall, spUfo, spWave, spRobot, spSpider}) {
        obj->setVisible(false);
    }

    requiredPlayer->setVisible(true);
}

void RemotePlayer::updateData(PlayerAccountData data, bool areDefaults) {
    innerNode->removeAllChildren();
    if (labelName)
        labelName->removeFromParent();
    // this->removeChildByID("dankmeme.globed/player-name"); // geode bug xD

    if (!areDefaults) {
        isDefault = false;
    }

    // create icons
    spCube = SimplePlayer::create(data.cube);
    spCube->updatePlayerFrame(data.cube, IconType::Cube);

    spShip = SimplePlayer::create(data.ship);
    spShip->updatePlayerFrame(data.ship, IconType::Ship);

    spBall = SimplePlayer::create(data.ball);
    spBall->updatePlayerFrame(data.ball, IconType::Ball);

    spUfo = SimplePlayer::create(data.ufo);
    spUfo->updatePlayerFrame(data.ufo, IconType::Ufo);

    spWave = SimplePlayer::create(data.wave);
    spWave->updatePlayerFrame(data.wave, IconType::Wave);

    spRobot = SimplePlayer::create(data.robot);
    spRobot->updatePlayerFrame(data.robot, IconType::Robot);

    spSpider = SimplePlayer::create(data.spider);
    spSpider->updatePlayerFrame(data.spider, IconType::Spider);

    // get colors
    auto primary = GameManager::get()->colorForIdx(data.color1);
    auto secondary = GameManager::get()->colorForIdx(data.color2);

    if (isSecond) {
        setValuesAndAdd(secondary, primary); // swap colors for duals
    } else {
        setValuesAndAdd(primary, secondary);
    }

    lastMode = IconGameMode::NONE;

    // update name
    name = data.name;

    auto namesEnabled = Mod::get()->getSettingValue<bool>("show-names");
    if (!isSecond && namesEnabled) {
        auto nameOffset = Mod::get()->getSettingValue<int64_t>("show-names-offset");
        auto nameScale = Mod::get()->getSettingValue<double>("show-names-scale");

        labelName = CCLabelBMFont::create(name.c_str(), "chatFont.fnt");
        labelName->setID("dankmeme.globed/player-name");
        labelName->setAnchorPoint({0.5f, 0.5f});
        labelName->setScale(nameScale);
        labelName->setPosition({0.f, 0.f + nameOffset});
        labelName->setZOrder(99);
        this->addChild(labelName);
    }
}

void RemotePlayer::setRotationX(float x) { innerNode->setRotationX(x); }
void RemotePlayer::setRotationY(float y) { innerNode->setRotationY(y); }
void RemotePlayer::setRotation(float y) { innerNode->setRotation(y); }
void RemotePlayer::setScale(float scale) { innerNode->setScale(scale); }
void RemotePlayer::setScaleX(float scale) { innerNode->setScaleX(scale); }
void RemotePlayer::setScaleY(float scale) { innerNode->setScaleY(scale); }
float RemotePlayer::getRotationX() { return innerNode->getRotationX(); }
float RemotePlayer::getRotationY() { return innerNode->getRotationY(); }
float RemotePlayer::getRotation() { return innerNode->getRotation(); }

RemotePlayer* RemotePlayer::create(bool isSecond, PlayerAccountData data) {
    auto ret = new RemotePlayer;
    if (ret && ret->init(data, isSecond)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void RemotePlayer::setValuesAndAdd(ccColor3B primary, ccColor3B secondary) {
    // sets colors and adds them this node
    for (SimplePlayer* obj : {spCube, spShip, spBall, spUfo, spWave, spRobot, spSpider}) {
        obj->setAnchorPoint({0.5f, 0.5f});
        obj->setColor(primary);
        obj->setSecondColor(secondary);
        obj->setVisible(false);
        innerNode->addChild(obj);
    }
}
