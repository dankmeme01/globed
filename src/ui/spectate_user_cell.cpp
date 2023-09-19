#include "spectate_user_cell.hpp"

bool SpectateUserCell::init(const CCSize& size, std::string name, SimplePlayer* cubeIcon) {
    if (!CCLayer::init()) return false;
    m_menu = CCMenu::create();
    m_name = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
    m_name->setPosition({cubeIcon->getContentSize().width + 25.f, size.height});
    m_name->setAnchorPoint({0.f, 0.5f});

    cubeIcon->setPosition({10.f, size.height});

    this->addChild(cubeIcon);
    this->addChild(m_name);

    return true;
}

SpectateUserCell* SpectateUserCell::create(const CCSize& size, std::string name, SimplePlayer* cubeIcon) {
    auto ret = new SpectateUserCell;
    if (ret && ret->init(size, name, cubeIcon)) {
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}