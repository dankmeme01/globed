#pragma once
#include <Geode/Geode.hpp>

constexpr float SPP_CELL_HEIGHT = 45.0f;
const cocos2d::CCSize SPP_LIST_SIZE = { 338.f, 150.f };

using namespace geode::prelude;

class SpectatePopup : public geode::Popup<> {
protected:
    GJCommentListLayer* m_list;
    bool setup() override;
public:
    static SpectatePopup* create();

    // bad code
    void closeAndResume(CCObject* sender);
};