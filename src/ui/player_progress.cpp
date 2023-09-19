#include "player_progress.hpp"
#include "../global_data.hpp"

bool PlayerProgress::init(int playerId_) {
    if (!CCNode::init()) return false;

    m_playerId = playerId_;

    updateValues(0.0f);

    return true;
}

void PlayerProgress::updateValues(float percentage) {
    auto cache = g_accDataCache.lock();
    std::string accName = "Player";
    if (cache->contains(m_playerId)) {
        auto account = cache->at(m_playerId);
        accName = account.name;
    }

    if (m_playerText) m_playerText->removeFromParent();

    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << percentage;
    auto val = stream.str();
    if (val.find(".") == std::string::npos) {
        val += ".0";
    }

    m_playerText = CCLabelBMFont::create(fmt::format("{} {}%", accName, val).c_str(), "chatFont.fnt");
    m_playerText->setAnchorPoint({0.f, 0.f});
    setContentSize(m_playerText->getContentSize());
    
    this->addChild(m_playerText);
}

PlayerProgress* PlayerProgress::create(int playerId_) {
    auto ret = new PlayerProgress;
    if (ret && ret->init(playerId_)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}