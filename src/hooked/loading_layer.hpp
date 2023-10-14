#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>

class $modify(LoadingLayer) {
    void loadingFinished() {
        // load death effects
        bool deathEffects = geode::Mod::get()->getSettingValue<bool>("death-effects");

        if (deathEffects) {
            geode::log::debug("Starting to load death effects");
            for (int i = 1; i < 17; i++) {
                geode::log::debug("Loading death effect {}", i);
                GameManager::get()->loadDeathEffect(i);
            }
            geode::log::debug("Finished loading death effects");
        }

        LoadingLayer::loadingFinished();
    }
};