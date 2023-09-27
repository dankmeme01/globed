#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <ui/menu.hpp>
#include <util.hpp>
#include <global_data.hpp>

using namespace geode::prelude;

class $modify(ModifiedMenuLayer, MenuLayer) {
    bool m_btnIsConnected;
    CCMenuItemSpriteExtra* m_globedBtn = nullptr;

    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        m_fields->m_btnIsConnected = false;
        addGlobedButton();

        if (GJAccountManager::sharedState()->m_accountID <= 0) {
            if (!g_shownAccountWarning) {
                g_errMsgQueue.push("You are not logged into a Geometry Dash account. Globed will not function until you log in.");
                g_shownAccountWarning = true;
            }
        } else {
            g_shownAccountWarning = false;
            g_accountData.lock() = PlayerAccountData {
                .cube = GameManager::get()->getPlayerFrame(),
                .ship = GameManager::get()->getPlayerShip(),
                .ball = GameManager::get()->getPlayerBall(),
                .ufo = GameManager::get()->getPlayerBird(),
                .wave = GameManager::get()->getPlayerDart(),
                .robot = GameManager::get()->getPlayerRobot(),
                .spider = GameManager::get()->getPlayerSpider(),
                .color1 = GameManager::get()->getPlayerColor(),
                .color2 = GameManager::get()->getPlayerColor2(),
                .glow = GameManager::get()->getPlayerGlow(),
                .name = GJAccountManager::sharedState()->m_username,
            };
        }
        sendMessage(NMMenuLayerEntry {});

        // process potential errors and check for button changes
        CCScheduler::get()->scheduleSelector(schedule_selector(ModifiedMenuLayer::updateStuff), this, 0.1f, false);

        return true;
    }

    void addGlobedButton() {
        if (m_fields->m_globedBtn) m_fields->m_globedBtn->removeFromParent();
        
        auto bottomMenu = this->getChildByID("bottom-menu");
    
        auto sprite = CircleButtonSprite::createWithSpriteFrameName(
            "menuicon.png"_spr,
            1.f,
            m_fields->m_btnIsConnected ? CircleBaseColor::Cyan : CircleBaseColor::Green,
            CircleBaseSize::MediumAlt);

        m_fields->m_globedBtn = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(ModifiedMenuLayer::onGlobedMenuButton)
            );


        m_fields->m_globedBtn->setID("dankmeme.globed/main-menu-button");

        bottomMenu->addChild(m_fields->m_globedBtn);
        bottomMenu->updateLayout();
    }

    void updateStuff(float unused) {
        globed_util::handleErrors();

        if (g_networkHandler->established() != m_fields->m_btnIsConnected) {
            m_fields->m_btnIsConnected = g_networkHandler->established();
            addGlobedButton();
        }
    }

    void sendMessage(NetworkThreadMessage msg) {
        g_netMsgQueue.push(msg);
    }

    void onGlobedMenuButton(CCObject* sender) {
        auto director = CCDirector::get();
        auto layer = GlobedMenuLayer::create();
        layer->setID("dankmeme.globed/layer-globed-menu");
        
        auto destScene = globed_util::sceneWithLayer(layer);

        auto transition = CCTransitionFade::create(0.5f, destScene);
        director->pushScene(transition);
    }
};
