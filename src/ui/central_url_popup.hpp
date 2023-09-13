#pragma once
#include "Geode/binding/TextArea.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class CentralUrlPopup : public geode::Popup<> {
protected:
    CCTextInputNode* m_curlEntry;
    bool setup() override {
        auto winSize = CCDirector::get()->getWinSize();

        setTitle("Globed server configuration");

        CCPoint topPos = {winSize.width / 2, winSize.height / 2 + m_size.height / 2};
        
        auto curlHeader = CCLabelBMFont::create("Central server URL", "bigFont.fnt");
        curlHeader->setAnchorPoint({0.5f, 1.f});
        curlHeader->setScale(0.6f);
        curlHeader->setPosition({topPos.x, topPos.y - 32.f});
        m_mainLayer->addChild(curlHeader);

        auto previousValue = Mod::get()->getSavedValue<std::string>("central");

        // auto curlEntry = TextArea::create("Testing stuff yoo uwu", "chatFont.fnt", 1.f, m_size.width * 0.8f, {0.5f, 0.5f}, 10.f, true);
        m_curlEntry = CCTextInputNode::create(m_size.width * 0.8f, 32.f, "http://example.com", "chatFont.fnt");
        m_curlEntry->setMaxLabelWidth(40);
        m_curlEntry->setAnchorPoint({0.5f, 0.5f});
        m_curlEntry->setAllowedChars("abcdefghijklmnopqrstuvwxyz1234567890.:/");
        m_curlEntry->setLabelPlaceholderColor({130, 130, 130});
        m_curlEntry->setPosition({topPos.x, winSize.height / 2});
        m_curlEntry->setString(previousValue);
        this->addChild(m_curlEntry);

        auto btnMenu = CCMenu::create();
        auto btnSprite = ButtonSprite::create("Apply", "goldFont.fnt", "GJ_button_01.png", .8f);
        auto applyButton = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(CentralUrlPopup::apply));
        applyButton->setAnchorPoint({0.5f, 0.f});
        applyButton->setPosition({0, -m_size.height / 2 + 10.f});
        btnMenu->addChild(applyButton);

        m_mainLayer->addChild(btnMenu);
        return true;
    }

    void apply(CCObject* sender) {
        Mod::get()->setSavedValue("central", std::string(m_curlEntry->getString()));
        onClose(sender);
    }
public:
    static CentralUrlPopup* create() {
        auto ret = new CentralUrlPopup;
        if (ret && ret->init(360.f, 240.f)) {
            ret->autorelease();
            return ret;
        }

        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};