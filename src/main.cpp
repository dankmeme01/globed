#include <limits>
#include <random>
#include <chrono>

#include <Geode/Geode.hpp>
#include <hooked/hooked.hpp>
#include <net/network_handler.hpp>
#include <util.hpp>

using namespace geode::prelude;

// this is hacky but uhh yeah thanks alk
class DummyErrorCheckNode : public CCNode {
public:
    bool init() {
        if (!CCNode::init()) return false;

        CCScheduler::get()->scheduleSelector(schedule_selector(DummyErrorCheckNode::updateErrors), this, 0.25f, false);
        return true;
    }

    void updateErrors(float _unused) {
        auto* currentScene = CCDirector::get()->getRunningScene();
        if (!currentScene || !currentScene->getChildren() || currentScene->getChildrenCount() == 0) return;

        auto* currentLayer = currentScene->getChildren()->objectAtIndex(0);
        
        if (typeinfo_cast<CCTransitionScene*>(currentScene)
            || typeinfo_cast<LoadingLayer*>(currentLayer)) {}

        else {
            globed_util::handleErrors();
        }
    }

    static DummyErrorCheckNode* create() {
        auto* ret = new DummyErrorCheckNode;
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

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

    // this is hacky but uhh yeah thanks alk x2
    auto errcheckNode = DummyErrorCheckNode::create();
    errcheckNode->setID("dankmeme.globed/err-check-node");
    SceneManager::get()->keepAcrossScenes(errcheckNode);

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