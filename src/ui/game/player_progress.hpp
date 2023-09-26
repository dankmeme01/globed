#pragma once
#include "player_progress_base.hpp"
#include <data/player_account_data.hpp>

using namespace geode::prelude;

class PlayerProgress : public PlayerProgressBase {
protected:
    int m_playerId;
    float m_prevPercentage;
    std::string m_playerName;
    
    CCLabelBMFont* m_playerText = nullptr;
    CCSprite* m_playerArrow = nullptr;
    bool m_prevRightSide = false, m_firstTick = true;
    unsigned char m_progressOpacity;
    RowLayout* m_layout;

    bool init(int playerId_);
public:
    bool m_isDefault;
    void updateValues(float percentage, bool onRightSide);
    void updateData(const PlayerAccountData& data);
    static PlayerProgress* create(int playerId_);
};