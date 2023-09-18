#include "globed_level_cell.hpp"

void GlobedLevelCell::updatePlayerCount(unsigned short count) {
    if (m_playerCount) {
        m_playerCount->removeFromParent();
    }

    m_playerCount = cocos2d::CCLabelBMFont::create(fmt::format("{} players", count).c_str(), "goldFont.fnt");
    
    m_playerCount->setScale(0.5);
    m_playerCount->setAnchorPoint({1.f, 0.5f});
    m_playerCount->setPosition({m_width - 8.f, 70.f});
    this->addChild(m_playerCount);
}