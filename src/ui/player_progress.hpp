#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class PlayerProgress : public CCNode {
protected:
    int m_playerId;
    CCLabelBMFont* m_playerText = nullptr;

    bool init(int playerId_);
public:
    void updateValues(float percentage);
    static PlayerProgress* create(int playerId_);
};