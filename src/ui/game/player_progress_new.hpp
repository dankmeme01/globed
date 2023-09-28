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

    float m_prevIconScale = 0.6f;
    
    SimplePlayer* m_playerIcon = nullptr;
    CCLayerColor* m_line = nullptr;
    ccColor4B m_lineColor = {255, 255, 255, 255};

    // settings
    float m_piOffset;

    bool init(int playerId_, float piOffset_);
public:
    void updateValues(float percentage, bool onRightSide);
    void updateData(const PlayerAccountData& data);
    void updateDataWithDefaults();
    void setIconScale(float scale);

    void hideLine();
    void showLine();
    static PlayerProgressNew* create(int playerId_, float piOffset_);
};