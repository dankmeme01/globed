#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "../ui/spectate_popup.hpp"

using namespace geode::prelude;

class $modify(ModifiedPauseLayer, PauseLayer) {
    void addSpectateButton() {
        auto menu = CCMenu::create();

        auto spectateSprite = CCSprite::createWithSpriteFrameName("GJ_infoBtn_001.png");
        spectateSprite->setScale(0.65f);
        auto spectateButton = CCMenuItemSpriteExtra::create(spectateSprite, this, menu_selector(ModifiedPauseLayer::onSpectate));
        menu->addChild(spectateButton);

        auto winSize = CCDirector::get()->getWinSize();
        spectateButton->setAnchorPoint({1.f, 0.f});
        spectateButton->setPosition({
            winSize.width / 2 - spectateButton->getContentSize().width / 2 - 10.f,
            -winSize.height / 2 + spectateButton->getContentSize().height / 2 + 10.f
        });

        spectateButton->setID("dankmeme.globed/spectate-button");
        
        this->addChild(menu);
    }

    static PauseLayer* create(bool p0) {
        auto instance = new ModifiedPauseLayer;
        if (instance && instance->init()) {
            instance->autorelease();
            // instance->addSpectateButton();
            return instance;
        }
        CC_SAFE_DELETE(instance);
        return nullptr;
    }

    void onSpectate(CCObject* sender) {
        SpectatePopup::create()->show();
    }
};