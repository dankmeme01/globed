#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

#include <global_data.hpp>
#include <ui/levels/globed_levels_layer.hpp>

class $modify(ModifiedLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level) {
        if (!LevelInfoLayer::init(level)) return false;

        if (!g_networkHandler->established()) {
            return true;
        }

        auto backMenu = getChildByID("left-side-menu");

        // add a button for viewing levels
        auto levelsSprite = CircleButtonSprite::createWithSpriteFrameName("menuicon.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Medium);
        // levelsSprite->setScale(0.8f);
        auto levelsButton = CCMenuItemSpriteExtra::create(levelsSprite, this, menu_selector(ModifiedLevelInfoLayer::onOpenLevelsButton));
        backMenu->addChild(levelsButton);

        backMenu->updateLayout();

        // error checking
        CCScheduler::get()->scheduleSelector(schedule_selector(ModifiedLevelInfoLayer::checkErrors), this, 0.1f, false);

        return true;
    }

    void onOpenLevelsButton(CCObject* sender) {
        auto director = CCDirector::get();
        auto layer = GlobedLevelsLayer::create();
        layer->setID("dankmeme.globed/layer-globed-levels");
        
        auto destScene = globed_util::sceneWithLayer(layer);

        auto transition = CCTransitionFade::create(0.5f, destScene);
        director->pushScene(transition);
    }

    void checkErrors(float dt) {
        globed_util::handleErrors();
    }
};