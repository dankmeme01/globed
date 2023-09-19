#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class SpectateUserCell : public CCLayer {
protected:
    CCMenu* m_menu;
    CCLabelBMFont* m_name;
    CCMenuItemSpriteExtra* m_btnSpectate;

    bool init(const CCSize& size, std::string name, SimplePlayer* cubeIcon);

public:
    static SpectateUserCell* create(const CCSize& size, std::string name, SimplePlayer* cubeIcon);
};