#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <global_data.hpp>

using namespace geode::prelude;

class $modify(ModifiedGJBGL, GJBaseGameLayer) {
    // pushButton and releaseButton are for disabling player movement
    void pushButton(int p0, bool p1) {
        //TODO: make the text input not pass through touches so this doesn't get triggered
        /*auto playlayer = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
        if (playlayer != nullptr)
            playlayer->deselectMessageInput();*/

        if (g_spectatedPlayer == 0) GJBaseGameLayer::pushButton(p0, p1);
    }

    void releaseButton(int p0, bool p1) {
        if (g_spectatedPlayer == 0) GJBaseGameLayer::releaseButton(p0, p1);
    }
};