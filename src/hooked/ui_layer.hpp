#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/UILayer.hpp>

using namespace geode::prelude;

class $modify(UILayer) {
    void keyDown(cocos2d::enumKeyCodes key) {
        //TODO the text input doesn't forward keyboard inputs to uilayer and such
        //     so this doesn't work at all
        log::debug("key down {}", key);
        if (key == cocos2d::enumKeyCodes::KEY_Enter) {
            auto* playlayer = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
            if (playlayer != nullptr)
                playlayer->onSendMessage(playlayer);
        }

        UILayer::keyDown(key);
    }
};