#include "spectate_user_cell.hpp"
#include "spectate_popup.hpp"

#include <global_data.hpp>

bool SpectateUserCell::init(const CCSize& size, std::string name, SimplePlayer* cubeIcon, int playerId, SpectatePopup* popup) {
    if (!CCLayer::init()) return false;
    m_playerId = playerId;
    m_popup = popup;

    m_menu = CCMenu::create();
    m_menu->setPosition({size.width, size.height});
    m_name = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
    m_name->setPosition({25.f + cubeIcon->getScaledContentSize().width + 25.f, size.height - 9.f});
    m_name->setAnchorPoint({0.f, 1.0f});
    m_name->setScale(0.8f);

    cubeIcon->setPosition({25.f, size.height - 22.f});

    this->addChild(cubeIcon);
    this->addChild(m_name);
    this->addChild(m_menu);

    // add button
    m_isSpectated = g_spectatedPlayer == playerId;

    auto sprName = m_isSpectated ? "spectate-stop.png"_spr : "spectate.png"_spr;
    auto sprColor = m_isSpectated ? CircleBaseColor::Gray : CircleBaseColor::Green;

    auto btnSprite = CircleButtonSprite::createWithSpriteFrameName(sprName, 1.f, sprColor, CircleBaseSize::Medium);
    btnSprite->setScale(0.75f);
    auto btn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(SpectateUserCell::onSpectate));
    btn->setPosition({-30.f, -23.f});

    m_menu->addChild(btn);

    return true;
}

void SpectateUserCell::onSpectate(CCObject* sender) {
    if (m_isSpectated) {
        g_spectatedPlayer = 0;
    } else {
        g_spectatedPlayer = m_playerId;
    }

    m_popup->closeAndResume(sender);
}

SpectateUserCell* SpectateUserCell::create(const CCSize& size, std::string name, SimplePlayer* cubeIcon, int playerId, SpectatePopup* popup) {
    auto ret = new SpectateUserCell;
    if (ret && ret->init(size, name, cubeIcon, playerId, popup)) {
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}