#include <Geode/Geode.hpp>
#include <limits>
#include <random>
#include <chrono>

#include "hooked/hooked.hpp"
#include "net/network_handler.hpp"

using namespace geode::prelude;

// void onExit() {
//     geode::log::debug("atexit");
//     g_isModLoaded = false;
//     g_modLoadedCv.notify_all();

//     geode::log::debug("notified now sleeping");
//     std::this_thread::sleep_for(std::chrono::seconds(3));
// }

$on_mod(Loaded) {
    // std::atexit(onExit);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    
    g_secretKey = distrib(gen);

    g_networkHandler = std::make_shared<NetworkHandler>();
    g_networkHandler->run();
}