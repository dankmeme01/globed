#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "Geode/loader/Log.hpp"
#include "util.hpp"
#include "GlobedMenuLayer.hpp"

using namespace geode::prelude;

class $modify(ModifiedMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto bottomMenu = this->getChildByID("bottom-menu");
        
        auto menuButtonSprite = CircleButtonSprite::createWithSprite(
            "globedMenuIcon.png"_spr,
            1.f,
            CircleBaseColor::Cyan);

        if (menuButtonSprite == nullptr) {
            log::error("menuButtonSprite is nullptr, brace yourself for a crash..");
        }

        auto menuButton = CCMenuItemSpriteExtra::create(
            menuButtonSprite,
            this,
            menu_selector(ModifiedMenuLayer::onGlobedMenuButton)
            );

        menuButton->setID("dankmeme.globed/main-menu-button");

        bottomMenu->addChild(menuButton);
        bottomMenu->updateLayout();

        return true;
    }

    void onGlobedMenuButton(CCObject* sender) {
        // FLAlertLayer::create("Button clicked", "Yo", "OK")->show();
        auto director = CCDirector::get();
        auto layer = GlobedMenuLayer::create();
        layer->setID("dankmeme.globed/layer-globed-menu");
        auto destScene = globed_util::sceneWithLayer(layer);

        auto transition = CCTransitionFade::create(0.5f, destScene, ccBLACK);
        director->pushScene(transition);
    }
};
