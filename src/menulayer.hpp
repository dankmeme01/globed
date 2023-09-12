#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "ui/globed_menu_layer.hpp"
#include "util.hpp"
#include "global_data.hpp"

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

        // process potential errors

        g_accountID = GJAccountManager::sharedState()->m_accountID;
        sendMessage(GameLoadedData {});
        CCScheduler::get()->scheduleSelector(schedule_selector(ModifiedMenuLayer::checkErrors), this, 0.0f, false);

        return true;
    }

    void checkErrors(float unused) {
        globed_util::handleErrors();
    }

    void sendMessage(Message msg) {
        std::lock_guard<std::mutex> lock(g_netMutex);
        g_netMsgQueue.push(msg);
        g_netCVar.notify_one();
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
