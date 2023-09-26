/*
* GlobedLevelCell is a simple LevelCell with an addition of a player count.
*/

#pragma once

class GlobedLevelCell : public LevelCell {
public:
    inline GlobedLevelCell(char const* identifier, float parentHeight, float height) : LevelCell(identifier, parentHeight, height) {}
    void updatePlayerCount(unsigned short count);
protected:
    cocos2d::CCLabelBMFont* m_playerCount = nullptr;
};