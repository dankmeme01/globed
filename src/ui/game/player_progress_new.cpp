#include "player_progress_new.hpp"
#include <global_data.hpp>

bool PlayerProgressNew::init(int playerId_, float piOffset_) {
    if (!CCNode::init()) return false;

    m_playerId = playerId_;
    m_piOffset = piOffset_;
    m_progressOpacity = static_cast<unsigned char>(Mod::get()->getSettingValue<int64_t>("show-progress-opacity"));
    m_isDefault = true;

    auto pScale = static_cast<float>(Mod::get()->getSettingValue<int64_t>("show-progress-offset"));

    updateDataWithDefaults();

    return true;
}

void PlayerProgressNew::updateValues(float _unused1, bool _unused2) {}

void PlayerProgressNew::updateData(const PlayerAccountData& data) {
    m_isDefault = false;

    if (m_playerIcon) m_playerIcon->removeFromParent();
    m_playerIcon = SimplePlayer::create(data.cube);
    m_playerIcon->setColor(GameManager::get()->colorForIdx(data.color1));
    m_playerIcon->setSecondColor(GameManager::get()->colorForIdx(data.color2));
    m_playerIcon->setGlowOutline(data.glow);
    m_playerIcon->updatePlayerFrame(data.cube, IconType::Cube);
    m_playerIcon->setScale(m_prevIconScale);
    m_playerIcon->setAnchorPoint({0.5f, 1.f});
    m_playerIcon->setPosition({0.f, -30.f - m_piOffset});
    this->addChild(m_playerIcon);

    // auto color1 = GameManager::get()->colorForIdx(data.color1);
    auto color1 = GameManager::get()->colorForIdx(m_altColor ? data.color1 : data.color2);
    m_lineColor = {.r = color1.r, .g = color1.g, .b = color1.b, .a = 255};

    if (m_line) m_line->removeFromParent();

    m_line = CCLayerColor::create(m_lineColor, 2.f, 8.f);
    m_line->setPosition({0.f, -4.f});

    this->addChild(m_line);
}

void PlayerProgressNew::updateDataWithDefaults() {
    updateData(DEFAULT_PLAYER_ACCOUNT_DATA);
    m_isDefault = true;
}

void PlayerProgressNew::setIconScale(float scale) {
    m_prevIconScale = scale;
    if (m_playerIcon) m_playerIcon->setScale(scale);
}

void PlayerProgressNew::hideLine() {
    if (m_line) m_line->setVisible(false);
}

void PlayerProgressNew::showLine() {
    if (m_line) m_line->setVisible(true);
}

void PlayerProgressNew::setAltLineColor(bool alt) {
    m_altColor = alt;
}

PlayerProgressNew* PlayerProgressNew::create(int playerId_, float piOffset_) {
    auto ret = new PlayerProgressNew;
    if (ret && ret->init(playerId_, piOffset_)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}