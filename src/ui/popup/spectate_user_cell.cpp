#include "spectate_user_cell.hpp"
#include "spectate_popup.hpp"

#include <global_data.hpp>
#define GLOBED_SPECTATE_ENABLED 1

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

    // add spectate button
#ifdef GLOBED_SPECTATE_ENABLED
    m_isSpectated = g_spectatedPlayer == playerId;
    auto sprName = m_isSpectated ? "spectate-stop.png"_spr : "spectate.png"_spr;

    auto sprColor = m_isSpectated ? CircleBaseColor::Gray : CircleBaseColor::Green;
    if (playerId != 0) {
        auto btnSprite = CircleButtonSprite::createWithSpriteFrameName(sprName, 1.f, sprColor, CircleBaseSize::Medium);
        btnSprite->setScale(0.75f);
        auto btn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(SpectateUserCell::onSpectate));
        btn->setPosition({-30.f, -23.f});

        m_menu->addChild(btn);
    }
#endif

    return true;
}

void SpectateUserCell::onSpectate(CCObject* sender) {
    auto seenPopup = Mod::get()->getSavedValue<int64_t>("seen-spectate-popup") == 69;

    if (!seenPopup) {
        Mod::get()->setSavedValue("seen-spectate-popup", static_cast<int64_t>(69));
        FLAlertLayer::create(
            "Warning",
            "Spectator mode is <cy>experimental</c>. It might not look smooth, and depending on the level it might look <cr>completely broken</c>. This warning appears <cy>only once</c>.",
            "OK"
        )->show();
        return;
    }

    if (auto pl = PlayLayer::get()) {
        if (pl->m_level->m_levelID == 95174837) {
            bool seenWhat = Mod::get()->getSavedValue<int64_t>("seen-spectate-what-popup") == 69;
            if (!seenWhat) {
                Mod::get()->setSavedValue("seen-spectate-what-popup", static_cast<int64_t>(69));
                FLAlertLayer::create(
                    "Warning",
                    "Thank you for trying to break my mod! (spectating is probably not gonna work on this level). You're free to press the button again and try it anyway.",
                    "OK"
                )->show();
                return;
            }
        }
    }

    if (m_isSpectated) {
        g_spectatedPlayer = 0;
    } else {
        g_spectatedPlayer = m_playerId;
        log::debug("start spectating {}", m_playerId);
    }

    if (auto pl = PlayLayer::get()) {
        pl->togglePracticeMode(false);
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