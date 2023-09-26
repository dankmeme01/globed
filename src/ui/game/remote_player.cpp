#include "remote_player.hpp"
#include "name_colors.hpp"
#include <util.hpp>

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

bool RemotePlayer::init(PlayerAccountData data, bool isSecond_, RemotePlayerSettings settings_) {
    if (!CCNode::init()) {
        return false;
    }

    isSecond = isSecond_;

    innerNode = CCNode::create();
    innerNode->setAnchorPoint({0.5f, 0.5f});
    this->addChild(innerNode);

    isDefault = data == DEFAULT_DATA;

    checkpointNode = CCSprite::createWithSpriteFrameName("checkpoint_01_001.png");
    checkpointNode->setZOrder(1);
    checkpointNode->setID("dankmeme.globed/remote-player-practice");

    this->addChild(checkpointNode);

    settings = settings_;

    updateData(data, isDefault);

    if ((!settings.practiceIcon) || (!settings.secondNameEnabled && isSecond)) {
        checkpointNode->setVisible(false);
    }

    return true;
}

void RemotePlayer::tick(const SpecificIconData& data, bool practice) {
    if (data.gameMode != lastMode) {
        lastMode = data.gameMode;
        setActiveIcon(lastMode);
    }

    if (settings.defaultMiniIcons && (data.isMini != wasMini || firstTick)) {
        wasMini = data.isMini;
        spCube->updatePlayerFrame(wasMini ? DEFAULT_DATA.cube : realCube, IconType::Cube);
        spBall->updatePlayerFrame(wasMini ? DEFAULT_DATA.ball : realBall, IconType::Ball);
    }

    if (settings.practiceIcon && (practice != wasPractice || firstTick)) {
        wasPractice = practice;
        if (!settings.secondNameEnabled && isSecond) practice = false;

        checkpointNode->setVisible(practice);
    }

    if (data.isGrounded != wasGrounded || firstTick) {
        wasGrounded = data.isGrounded;
        spRobot->m_robotSprite->tweenToAnimation(wasGrounded ? "run" : "fall_loop", 0.2f);
        spSpider->m_spiderSprite->tweenToAnimation(wasGrounded ? "run" : "fall_loop", 0.2f);
    }

    firstTick = false;
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

    if (!areDefaults) {
        isDefault = false;
    }

    // create icons
    spCube = SimplePlayer::create(data.cube);
    spCube->updatePlayerFrame(data.cube, IconType::Cube);
    spCube->setID("dankmeme.globed/remote-player-cube");
    realCube = data.cube;

    spShip = SimplePlayer::create(data.ship);
    spShip->updatePlayerFrame(data.ship, IconType::Ship);
    spShip->setID("dankmeme.globed/remote-player-ship");
    spShip->setPosition({0.f, -5.f});
    
    spShipPassenger = SimplePlayer::create(data.cube);
    spShipPassenger->updatePlayerFrame(data.cube, IconType::Cube);
    spShipPassenger->setID("dankmeme.globed/remote-player-ship-passenger");
    spShipPassenger->setPosition({0.f, 10.f});
    spShipPassenger->setScale(0.55f);
    spShip->addChild(spShipPassenger);

    spBall = SimplePlayer::create(data.ball);
    spBall->updatePlayerFrame(data.ball, IconType::Ball);
    spBall->setID("dankmeme.globed/remote-player-ball");
    realBall = data.ball;

    spUfo = SimplePlayer::create(data.ufo);
    spUfo->updatePlayerFrame(data.ufo, IconType::Ufo);
    spUfo->setID("dankmeme.globed/remote-player-ufo");

    spUfoPassenger = SimplePlayer::create(data.cube);
    spUfoPassenger->updatePlayerFrame(data.cube, IconType::Cube);
    spUfoPassenger->setID("dankmeme.globed/remote-player-ufo-passenger");
    spUfoPassenger->setPosition({0.f, 5.f});
    spUfoPassenger->setScale(0.55f);
    spUfo->addChild(spUfoPassenger);

    spWave = SimplePlayer::create(data.wave);
    spWave->updatePlayerFrame(data.wave, IconType::Wave);
    spWave->setID("dankmeme.globed/remote-player-wave");

    spRobot = SimplePlayer::create(data.robot);
    spRobot->updatePlayerFrame(data.robot, IconType::Robot);
    spRobot->setID("dankmeme.globed/remote-player-robot");

    spSpider = SimplePlayer::create(data.spider);
    spSpider->updatePlayerFrame(data.spider, IconType::Spider);
    spSpider->setID("dankmeme.globed/remote-player-spider");

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
    auto nameScale = Mod::get()->getSettingValue<double>("show-names-scale");
    auto nameOffset = Mod::get()->getSettingValue<int64_t>("show-names-offset");

    if (isSecond) {
        nameOffset *= -1; // reverse direction for dual
    }

    if (!namesEnabled) {
        if (settings.practiceIcon) {
            if (isSecond && !settings.secondNameEnabled) return;

            // if no names, just put the practice icon above the player's head
            checkpointNode->setScale(nameScale * 0.8);
            checkpointNode->setPosition({0.f, 0.f + nameOffset});
            return;
        }
    } else {
        if (isSecond && !settings.secondNameEnabled) return;

        auto cpNodeWidth = checkpointNode->getContentSize().width;

        labelName = CCLabelBMFont::create(name.c_str(), "chatFont.fnt");
        labelName->setID("dankmeme.globed/remote-player-name");
        labelName->setPosition({0.f, 0.f + nameOffset});
        labelName->setScale(nameScale);
        labelName->setZOrder(1);
        labelName->setOpacity(settings.nameOpacity);
        if (settings.nameColors) {
            labelName->setColor(pickNameColor(name));
        }
        this->addChild(labelName);

        if (settings.practiceIcon) {
            checkpointNode->setScale(nameScale * 0.8);
            checkpointNode->setPosition({-(labelName->getContentSize().width / 2) - cpNodeWidth / 2, 0.f + nameOffset});
        }
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

void RemotePlayer::setOpacity(unsigned char opacity) {
    for (SimplePlayer* obj : {spCube, spShip, spBall, spUfo, spWave, spRobot, spSpider, spShipPassenger, spUfoPassenger}) {
        obj->setOpacity(opacity);
    }
}

RemotePlayer* RemotePlayer::create(bool isSecond, RemotePlayerSettings settings_, PlayerAccountData data) {
    auto ret = new RemotePlayer;
    if (ret && ret->init(data, isSecond, settings_)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void RemotePlayer::setValuesAndAdd(ccColor3B primary, ccColor3B secondary) {
    // sets colors and adds them this node
    for (SimplePlayer* obj : {spCube, spShip, spBall, spUfo, spWave, spRobot, spSpider}) {
        obj->setColor(primary);
        obj->setSecondColor(secondary);
        obj->setVisible(false);
        innerNode->addChild(obj);
    }

    // passengers
    for (SimplePlayer* obj : {spShipPassenger, spUfoPassenger}) {
        obj->setColor(primary);
        obj->setSecondColor(secondary);
    }
}
