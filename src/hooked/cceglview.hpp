#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCEGLView.hpp>

using namespace geode::prelude;

class $modify(CCEGLView) {
     void onGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ENTER) {
            auto* playlayer = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
            if (playlayer != nullptr)
                playlayer->onSendMessage(playlayer);
        }

        CCEGLView::onGLFWKeyCallback(window, key, scancode, action, mods);
    }
};