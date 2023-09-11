#pragma once
#include <Geode/Geode.hpp>
#include "global_data.hpp"

using namespace geode::prelude;

#define DEFAULT_CREATE(className) \
    static className* create() { \
        className* ret = new className(); \
        if (ret && ret->init()) { \
            ret->autorelease(); \
            return ret; \
        } \
        CC_SAFE_DELETE(ret); \
        return nullptr; \
    }

#define DEFAULT_KEYDOWN \
    void keyDown(enumKeyCodes key) { \
        globed_util::ui::handleDefaultKey(key); \
    }

#define DEFAULT_GOBACK \
    void goBack(CCObject* _sender) { \
        globed_util::ui::navigateBack(); \
    }

namespace globed_util {
    CCScene* sceneWithLayer(CCNode* layer);
    void handleErrors();

    inline void errorPopup(const std::string& text) {
        FLAlertLayer::create("Globed Error", text, "Ok")->show();
    }

    namespace net {
        bool updateGameServers(const std::string& url);
        bool connectToServer(const std::string& id);
    }

    namespace ui {
        void addBackground(CCNode* parent);
        void addBackButton(CCNode* parent, CCMenu* menu, SEL_MenuHandler callback);

        inline void enableKeyboard(CCLayer* layer) {
            layer->setKeyboardEnabled(true);
            layer->setKeypadEnabled(true);
        }

        inline void navigateBack() {
            auto director = CCDirector::get();
            director->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
        }

        inline bool handleDefaultKey(enumKeyCodes key) {
            switch (key) {
                case enumKeyCodes::KEY_Escape: {
                    ui::navigateBack();
                }

                default: {
                    return false;
                }
            }

            return true;
        }

    }
}