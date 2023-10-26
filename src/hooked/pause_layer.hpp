#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <ui/popup/spectate_popup.hpp>
#include <global_data.hpp>

#include "play_layer.hpp"

using namespace geode::prelude;

class $modify(ModifiedPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        
        auto mpl = static_cast<ModifiedPlayLayer*>(PlayLayer::get());

        if (!g_networkHandler->established() || (mpl == nullptr ? (g_currentLevelId == 0) : (!mpl->m_fields->m_readyForMP))) {
            return;
        }

        // reset the text field
        if (mpl->m_fields->m_messageInput) {
            mpl->m_fields->m_messageInput->onClickTrackNode(false);
        }

        auto menu = CCMenu::create();

        auto spectateSprite = CCSprite::createWithSpriteFrameName("GJ_profileButton_001.png");
        spectateSprite->setScale(0.8f);
        auto spectateButton = CCMenuItemSpriteExtra::create(spectateSprite, this, menu_selector(ModifiedPauseLayer::onSpectate));
        menu->addChild(spectateButton);

        auto winSize = CCDirector::get()->getWinSize();
        spectateButton->setPosition({
            winSize.width / 2 - spectateButton->getContentSize().width,
            -winSize.height / 2 + spectateButton->getContentSize().height
        });

        spectateButton->setID("dankmeme.globed/spectate-button");
        
        this->addChild(menu);
    }

    void onPracticeMode(CCObject* sender) {
        if (g_spectatedPlayer == 0) PauseLayer::onPracticeMode(sender);
    }

    void onNormalMode(CCObject* sender) {
        if (g_spectatedPlayer == 0) PauseLayer::onNormalMode(sender);
    }

    void onRestart(CCObject* sender) {
        if (g_spectatedPlayer == 0) PauseLayer::onRestart(sender);
    }

    void onSpectate(CCObject* sender) {
        auto popup = SpectatePopup::create();
        popup->m_noElasticity = true; // this is because of a dumb bug
        popup->show();
    }
};