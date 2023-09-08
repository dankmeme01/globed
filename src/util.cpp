#include "util.hpp"

/* Returns a scene with just this layer */
namespace globed_util {
    CCScene* sceneWithLayer(CCNode* layer) {
        auto scene = CCScene::create();
        
        layer->setPosition({0, 0});
        scene->addChild(layer);

        return scene;
    }
    
    void errorPopup(const std::string& text) {
        FLAlertLayer::create("Error", text, "Ok")->show();
    }

    namespace ui {
        void addBackground(CCNode* layer) {
            auto windowSize = CCDirector::get()->getWinSize();

            auto bg = CCSprite::create("GJ_gradientBG.png");
            auto bgSize = bg->getTextureRect().size;

            bg->setAnchorPoint({ 0.0f, 0.0f });
            bg->setScaleX((windowSize.width + 10.0f) / bgSize.width);
            bg->setScaleY((windowSize.height + 10.0f) / bgSize.height);
            bg->setPosition({ -5.0f, -5.0f });
            bg->setColor({ 0, 102, 255 });
            bg->setZOrder(-1);

            layer->addChild(bg);
        }

        void addBackButton(CCNode* parent, CCMenu* menu, SEL_MenuHandler callback) {
            auto windowSize = CCDirector::get()->getWinSize();
            auto backBtn = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"),
                parent,
                callback
            );

            backBtn->setPosition(-windowSize.width / 2 + 25.0f, windowSize.height / 2 - 25.0f);

            menu->addChild(backBtn);
        }
    }
}