#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>

using namespace geode::prelude;

// Opening another PlayLayer while inside of an existing one (via user list)
// will cause a crash when exiting the 2nd PlayLayer and clicking back repeatedly.
// We disable the buttons that make it possible to open another level.

class $modify(ProfilePage) {
    static void onModify(auto& self) {
        // run after betterinfo and lists
        (void)self.setHookPriority("ProfilePage::loadPageFromUserInfo", -132456790);
    }

    // we disable my levels and comment history
    void onMyLevels(CCObject* o) {
        if (!PlayLayer::get()) return ProfilePage::onMyLevels(o);
        FLAlertLayer::create("Notice", "This button is disabled when inside of a level. Sorry for the inconvenience.", "Ok")->show();
    }

    void onCommentHistory(CCObject* o) {
        if (!PlayLayer::get()) return ProfilePage::onCommentHistory(o);
        FLAlertLayer::create("Notice", "This button is disabled when inside of a level. Sorry for the inconvenience.", "Ok")->show();
    }

    // disable betterinfo button
    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);
        if (!PlayLayer::get()) return;

        bool betterinfo = Loader::get()->getLoadedMod("cvolton.betterinfo") != nullptr;
        log::debug("bi: {}", betterinfo);

        if (betterinfo) {
            auto* lbb = m_buttonMenu->getChildByID("bi-leaderboard-button");
            if (lbb) lbb->setVisible(false);
        }

        // disable lists button (what if?)

        bool lists = Loader::get()->getLoadedMod("cvolton.lists") != nullptr;
        log::debug("lists: {}", lists);

        if (lists) {
            auto* lb = m_buttonMenu->getChildByID("cvolton.lists/my-lists");
            if (lb) lb->setVisible(false);
        }
    }

};