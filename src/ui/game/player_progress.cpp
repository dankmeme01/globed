#include "player_progress.hpp"
#include <global_data.hpp>

bool PlayerProgress::init(int playerId_) {
    if (!CCNode::init()) return false;

    m_playerId = playerId_;
    m_progressOpacity = static_cast<unsigned char>(Mod::get()->getSettingValue<int64_t>("show-progress-opacity"));
    m_isDefault = true;

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

    m_playerName = "Player";

    updateValues(0.0f, false);

    return true;
}

void PlayerProgress::updateValues(float percentage, bool onRightSide) {
    // round the percentage down
    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << percentage;
    auto val = stream.str();

    // things like 5.0f round down to just '5', add the '.0' for consistency
    if (val.find(".") == std::string::npos) {
        val += ".0";
    }

    m_playerText->setString(fmt::format("{} {}%", m_playerName, val).c_str());

    if (onRightSide != m_prevRightSide || m_firstTick) {
        m_firstTick = false;
        m_prevRightSide = onRightSide;
        m_layout->setAxisReverse(!onRightSide);

        m_playerArrow->setFlipX(onRightSide);
    }

    this->updateLayout();
    
    setContentSize(m_playerText->getContentSize() + CCPoint{m_playerArrow->getScaledContentSize().width, 0.f});

    m_prevPercentage = percentage;
}

void PlayerProgress::updateData(const PlayerAccountData& data) {
    m_isDefault = false;
    m_playerName = data.name;
    updateValues(m_prevPercentage, m_prevRightSide);
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