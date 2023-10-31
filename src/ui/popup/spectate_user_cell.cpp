#include "spectate_user_cell.hpp"
#include "spectate_popup.hpp"

#include <hooked/play_layer.hpp> // crying
#include <global_data.hpp>

bool SpectateUserCell::init(const CCSize& size, std::string name, SimplePlayer* cubeIcon, int playerId, SpectatePopup* popup) {
    if (!CCLayer::init()) return false;
    m_playerId = playerId;
    m_popup = popup;

    m_menu = CCMenu::create();
    m_menu->setPosition({size.width, size.height});
    m_name = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
    m_name->setPosition({25.f + cubeIcon->getScaledContentSize().width + 25.f, size.height - 9.f});
    m_name->limitLabelWidth(100.f, 1.0f, 0.5f);
    m_name->setAnchorPoint({0.f, 1.0f});
    m_name->setScale(0.8f);

    cubeIcon->setPosition({25.f, size.height - 22.f});

    this->addChild(cubeIcon);
    this->addChild(m_name);
    this->addChild(m_menu);

    m_isSpectated = g_spectatedPlayer == playerId;
    if (playerId == 0) {
        return true;
    }

    // add spectate button
    auto sprName = m_isSpectated ? "spectate-stop.png"_spr : "spectate.png"_spr;

    auto sprColor = m_isSpectated ? CircleBaseColor::Gray : CircleBaseColor::Green;

    auto btnSprite = CircleButtonSprite::createWithSpriteFrameName(sprName, 1.f, sprColor, CircleBaseSize::Medium);
    btnSprite->setScale(0.75f);
    auto btn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(SpectateUserCell::onSpectate));
    btn->setPosition({-30.f, -23.f});

    m_menu->addChild(btn);

    // add the button to block/unblock the user in the chat
    m_isBlocked = static_cast<ModifiedPlayLayer*>(PlayLayer::get())->isUserBlocked(playerId);
    this->refreshBlockButton();

    return true;
}

void SpectateUserCell::onBlock(CCObject* sender) {
    auto mpl = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
    m_isBlocked = !m_isBlocked;

    if (m_isBlocked) {
        mpl->chatBlockUser(m_playerId);
    } else {
        mpl->chatUnblockUser(m_playerId);
    }
}

void SpectateUserCell::refreshBlockButton() {
    if (m_btnBlock) {
        m_btnBlock->removeFromParent();
    }

    const char* sprName = m_isBlocked ? "accountBtn_requests_001.png" : "accountBtn_removeFriend_001.png";

    auto blockBtnSprite = CCSprite::createWithSpriteFrameName(sprName);
    blockBtnSprite->setScale(0.75f);
    m_btnBlock = CCMenuItemSpriteExtra::create(blockBtnSprite, this, menu_selector(SpectateUserCell::onBlock));
    m_btnBlock->setPosition({-50.f, -23.f});

    m_menu->addChild(m_btnBlock);
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