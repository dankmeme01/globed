#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <ui/popup/spectate_popup.hpp>
#include <global_data.hpp>

using namespace geode::prelude;

class $modify(ModifiedPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        
        if (!g_networkHandler->established() || g_currentLevelId == 0) {
            return;
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

    void onSpectate(CCObject* sender) {
        auto popup = SpectatePopup::create();
        popup->m_noElasticity = true; // this is because of a dumb bug
        popup->show();
    }
};