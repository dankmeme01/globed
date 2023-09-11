#include <Geode/Geode.hpp>
#include <limits>
#include <thread>
#include <random>

#include "menulayer.hpp"
#include "playlayer.hpp"
#include "playerobject.hpp"
#include "net/network_thread.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    std::thread thread(networkThread);
    thread.detach();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    
    g_secretKey = distrib(gen);
}

$on_mod(Unloaded) {
    g_isModLoaded = false;
}