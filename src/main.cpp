#include <Geode/Geode.hpp>
#include <thread>

#include "menulayer.hpp"
#include "playlayer.hpp"
#include "playerobject.hpp"
#include "network_thread.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    std::thread thread(networkThread);
    thread.detach();
}

$on_mod(Unloaded) {
    g_isModLoaded = false;
}