#pragma once
#include "Geode/binding/TextArea.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class CentralUrlPopup : public geode::Popup<> {
protected:
    CCTextInputNode* m_curlEntry;
    bool setup() override;

    void apply(CCObject* sender);
public:
    static CentralUrlPopup* create();
};