#include <Geode/Geode.hpp>
#include "util.hpp"

using namespace geode::prelude;

class GlobedMenuLayer : public CCLayer {
protected:
    bool init() {
        if (!CCLayer::init()) return false;
        auto windowSize = CCDirector::get()->getWinSize();

        globed_util::ui::addBackground(this);

        auto menu = CCMenu::create();

        globed_util::ui::addBackButton(this, menu, menu_selector(GlobedMenuLayer::goBack));
        globed_util::ui::enableKeyboard(this);

        // test label
        auto label = CCLabelBMFont::create("Juicy mod", "bigfont.fnt");
        label->setID("dankmeme.globed/labellol");
        label->setPosition(windowSize / 2);
        this->addChild(label);

        this->addChild(menu);

        return true;
    }

public:
    DEFAULT_GOBACK
    DEFAULT_KEYDOWN
    DEFAULT_CREATE(GlobedMenuLayer)
};