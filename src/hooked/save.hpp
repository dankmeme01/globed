#include <Geode/Geode.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include "../global_data.hpp"

using namespace geode::prelude;

class $modify(AppDelegate) {
    void trySaveGame() {
        {
            std::lock_guard lock(g_modLoadedMutex);

            g_isModLoaded = false;
            g_modLoadedCv.notify_all();
        }

        AppDelegate::trySaveGame();
    }
};