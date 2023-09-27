#pragma once
#include "player_progress_base.hpp"
#include <data/player_account_data.hpp>

/*
* PlayerProgressNew it shows players' icons under the progress bar unlike PlayerProgress,
* which shows names and percentage values at the edge of the screen.
*/

class PlayerProgressNew : public PlayerProgressBase {
protected:
    int m_playerId;
    float m_prevPercentage;
    unsigned char m_progressOpacity;
    
    SimplePlayer* m_playerIcon = nullptr;

    bool init(int playerId_);
public:
    void updateValues(float percentage, bool onRightSide);
    void updateData(const PlayerAccountData& data);
    void updateDataWithDefaults();
    static PlayerProgressNew* create(int playerId_);
};