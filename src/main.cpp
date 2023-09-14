#include <Geode/Geode.hpp>
#include <limits>
#include <random>
#include <chrono>

#include "hooked/hooked.hpp"
#include "net/network_handler.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    
    auto skey = distrib(gen);

    g_networkHandler = std::make_shared<NetworkHandler>(skey);
    g_networkHandler->run();
}