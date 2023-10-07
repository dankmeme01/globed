// idk if this is good practice (most likely not) but i think it works.
// i could probably replace it with a much cleaner version but if it aint broke dont fix it

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/modify/LeaderboardsLayer.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#ifndef GEODE_IS_MACOS
#include <Geode/modify/KeysLayer.hpp>
#endif

#include <util.hpp>

using namespace geode::prelude;
#define GENERATE_ERROR_CHECK_HOOK(className) \
class $modify(Modified##className, className) { \
public: \
    bool init() { \
        if (!className::init()) return false; \
        CCScheduler::get()->scheduleSelector(schedule_selector(Modified##className::checkErrors), this, 0.25f, false); \
        return true; \
    } \
    void checkErrors(float unused) { \
        globed_util::handleErrors(); \
    } \
};

#define GENERATE_ERROR_CHECK_HOOK_ARG1(className, arg1t) \
class $modify(Modified##className, className) { \
public: \
    bool init(arg1t arg1) { \
        if (!className::init(arg1)) return false; \
        CCScheduler::get()->scheduleSelector(schedule_selector(Modified##className::checkErrors), this, 0.25f, false); \
        return true; \
    } \
    void checkErrors(float unused) { \
        globed_util::handleErrors(); \
    } \
};

GENERATE_ERROR_CHECK_HOOK(LevelSelectLayer)
GENERATE_ERROR_CHECK_HOOK(CreatorLayer)
GENERATE_ERROR_CHECK_HOOK_ARG1(LevelBrowserLayer, GJSearchObject*)
GENERATE_ERROR_CHECK_HOOK(LevelSearchLayer)
GENERATE_ERROR_CHECK_HOOK_ARG1(LeaderboardsLayer, LeaderboardState)
GENERATE_ERROR_CHECK_HOOK(GJGarageLayer)
GENERATE_ERROR_CHECK_HOOK_ARG1(EditLevelLayer, GJGameLevel*)
GENERATE_ERROR_CHECK_HOOK_ARG1(LevelEditorLayer, GJGameLevel*)

#ifndef GEODE_IS_MACOS
GENERATE_ERROR_CHECK_HOOK(KeysLayer)
#endif