#include "central_url_popup.hpp"
#include "../global_data.hpp"
#include <regex>

bool CentralUrlPopup::setup() {
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
    m_mainLayer->addChild(m_curlEntry);

    auto btnMenu = CCMenu::create();
    auto btnSprite = ButtonSprite::create("Apply", "goldFont.fnt", "GJ_button_01.png", .8f);
    auto applyButton = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(CentralUrlPopup::apply));
    applyButton->setAnchorPoint({0.5f, 0.f});
    applyButton->setPosition({0, -m_size.height / 2 + 10.f});
    btnMenu->addChild(applyButton);

    m_mainLayer->addChild(btnMenu);
    return true;
}

void CentralUrlPopup::apply(CCObject* sender) {
    std::string newServerUrl(m_curlEntry->getString());
    std::regex serverPattern("^(https?://[^\\s]+[.][^\\s]+)$");

    if (!newServerUrl.empty() && !std::regex_match(newServerUrl, serverPattern)) {
        FLAlertLayer::create("Invalid URL", "The URL provided does not match the needed schema. It must be either a domain name (like <cy>http://example.com</c>) or an IP address (like <cy>http://127.0.0.1:41000</c>)", "OK")->show();
        return;
    }
    
    Mod::get()->setSavedValue("central", newServerUrl);
    g_netMsgQueue.push(NMCentralServerChanged { newServerUrl });
    onClose(sender);
}

CentralUrlPopup* CentralUrlPopup::create() {
    auto ret = new CentralUrlPopup;
    if (ret && ret->init(360.f, 240.f)) {
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);
    return nullptr;
}