#include "remote_player.hpp"
#include "../util.hpp"

bool RemotePlayer::init(PlayerIconsData icons) {
    if (!CCNode::init()) {
        return false;
    }

    updateIcons(icons);
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

void RemotePlayer::updateIcons(PlayerIconsData icons) {
    this->removeAllChildren();

    // create icons
    spCube = SimplePlayer::create(icons.cube);
    spCube->updatePlayerFrame(icons.cube, IconType::Cube);

    spShip = SimplePlayer::create(icons.ship);
    spShip->updatePlayerFrame(icons.ship, IconType::Ship);

    spBall = SimplePlayer::create(icons.ball);
    spBall->updatePlayerFrame(icons.ball, IconType::Ball);

    spUfo = SimplePlayer::create(icons.ufo);
    spUfo->updatePlayerFrame(icons.ufo, IconType::Ufo);

    spWave = SimplePlayer::create(icons.wave);
    spWave->updatePlayerFrame(icons.wave, IconType::Wave);

    spRobot = SimplePlayer::create(icons.robot);
    spRobot->updatePlayerFrame(icons.robot, IconType::Robot);

    spSpider = SimplePlayer::create(icons.spider);
    spSpider->updatePlayerFrame(icons.spider, IconType::Spider);

    // get colors
    auto primary = GameManager::get()->colorForIdx(icons.color1);
    auto secondary = GameManager::get()->colorForIdx(icons.color2);
    setValuesAndAdd(primary, secondary);
}

RemotePlayer* RemotePlayer::create(PlayerIconsData icons) {
    auto ret = new RemotePlayer;
    if (ret && ret->init(icons)) {
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
        this->addChild(obj);
    }
}
