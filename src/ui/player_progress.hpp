#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class PlayerProgress : public CCNode {
protected:
    int m_playerId;
    CCLabelBMFont* m_playerText = nullptr;
    CCSprite* m_playerArrow = nullptr;
    bool m_prevRightSide = false, m_firstTick = true;
    unsigned char m_progressOpacity;
    RowLayout* m_layout;

    bool init(int playerId_);
public:
    void updateValues(float percentage, bool onRightSide);
    static PlayerProgress* create(int playerId_);
};