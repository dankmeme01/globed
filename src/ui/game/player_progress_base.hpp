#pragma once
#include <Geode/Geode.hpp>
#include <data/player_account_data.hpp>

using namespace geode::prelude;

class PlayerProgressBase : public CCNode {
public:
    bool m_isDefault;
    // updateValues updates the percentage values and the position of the indicator on the screen
    // it is unused in PlayerProgressNew
    virtual void updateValues(float percentage, bool onRightSide) = 0;
    // updateData updates the player name and icons
    virtual void updateData(const PlayerAccountData& data) = 0;
};