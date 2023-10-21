#include <limits>
#include <random>
#include <chrono>

#include <Geode/Geode.hpp>
#include <hooked/hooked.hpp>
#include <net/network_handler.hpp>
#include <util.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    
    auto skey = distrib(gen);

    // check for central server
    auto central = Mod::get()->getSavedValue<std::string>("central");
    if (central.empty()) {
        Mod::get()->setSavedValue("central", std::string("http://globed.dankmeme.dev"));
    }

    g_debug = Mod::get()->getSettingValue<bool>("debug");

    if (g_debug) {
        log::info("!! GLOBED DEBUG MODE IS ENABLED !!");
        log::info("!! idk why it's all caps, it ain't that serious lol !!");
    }

#ifdef GEODE_IS_MACOS
    globed_util::loadDeathEffects();
#endif

    g_networkHandler = std::make_shared<NetworkHandler>(skey);
    g_networkHandler->run();
}