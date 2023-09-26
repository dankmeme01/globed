#include "player_progress_new.hpp"

#include <global_data.hpp>

bool PlayerProgressNew::init(int playerId_) {
    if (!CCNode::init()) return false;

    m_playerId = playerId_;
    m_progressOpacity = static_cast<unsigned char>(Mod::get()->getSettingValue<int64_t>("show-progress-opacity"));

    m_layout = RowLayout::create();
    m_layout->setAutoScale(false);
    m_layout->setAxisAlignment(AxisAlignment::Start);
    this->setLayout(m_layout);
    
    m_playerText = CCLabelBMFont::create("Player 0.0%", "chatFont.fnt");
    m_playerText->setAnchorPoint({0.f, 0.f});
    m_playerText->setOpacity(m_progressOpacity);
    this->addChild(m_playerText);

    m_playerArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    m_playerArrow->setAnchorPoint({0.f, 0.1f});
    m_playerArrow->setOpacity(m_progressOpacity);
    m_playerArrow->setScale(0.5f);
    this->addChild(m_playerArrow);

    updateValues(0.0f, false);

    return true;
}

void PlayerProgressNew::updateValues(float percentage, bool onRightSide) {
    // find player's name
    auto cache = g_accDataCache.lock();
    std::string accName = "Player";
    if (cache->contains(m_playerId)) {
        auto account = cache->at(m_playerId);
        accName = account.name;
    }
    cache.unlock();

    // round the percentage down
    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << percentage;
    auto val = stream.str();

    // things like 5.0f round down to just '5', add the '.0' for consistency
    if (val.find(".") == std::string::npos) {
        val += ".0";
    }

    m_playerText->setString(fmt::format("{} {}%", accName, val).c_str());

    if (onRightSide != m_prevRightSide || m_firstTick) {
        m_firstTick = false;
        m_prevRightSide = onRightSide;
        m_layout->setAxisReverse(!onRightSide);

        m_playerArrow->setFlipX(onRightSide);
    }

    this->updateLayout();
    
    setContentSize(m_playerText->getContentSize() + CCPoint{m_playerArrow->getScaledContentSize().width, 0.f});
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