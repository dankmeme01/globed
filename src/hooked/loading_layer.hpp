#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <util.hpp>

class $modify(LoadingLayer) {
    void loadingFinished() {
        globed_util::loadDeathEffects();
        LoadingLayer::loadingFinished();
    }
};