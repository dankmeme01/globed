#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class SpectatePopup;

class SpectateUserCell : public CCLayer {
protected:
    CCMenu* m_menu;
    CCLabelBMFont* m_name;
    CCMenuItemSpriteExtra *m_btnSpectate, *m_btnBlock = nullptr;
    int m_playerId;
    bool m_isSpectated, m_isBlocked;
    SpectatePopup* m_popup;

    bool init(const CCSize& size, std::string name, SimplePlayer* cubeIcon, int playerId, SpectatePopup* popup);
    void onSpectate(CCObject* sender);
    void onBlock(CCObject* sender);
    void refreshBlockButton();

public:
    static SpectateUserCell* create(const CCSize& size, std::string name, SimplePlayer* cubeIcon, int playerId, SpectatePopup* popup);
};