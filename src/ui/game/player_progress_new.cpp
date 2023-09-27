#include "player_progress_new.hpp"
#include <global_data.hpp>

bool PlayerProgressNew::init(int playerId_) {
    if (!CCNode::init()) return false;

    m_playerId = playerId_;
    m_progressOpacity = static_cast<unsigned char>(Mod::get()->getSettingValue<int64_t>("show-progress-opacity"));
    m_isDefault = true;

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
    m_playerIcon->updatePlayerFrame(data.cube, IconType::Cube);
    this->addChild(m_playerIcon);
}

void PlayerProgressNew::updateDataWithDefaults() {
    updateData(DEFAULT_PLAYER_ACCOUNT_DATA);
    m_isDefault = true;
}

PlayerProgressNew* PlayerProgressNew::create(int playerId_) {
    auto ret = new PlayerProgressNew;
    if (ret && ret->init(playerId_)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}