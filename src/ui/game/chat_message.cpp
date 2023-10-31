#include "chat_message.hpp"
#include "name_colors.hpp"

const float TEXT_SCALE = 0.45f;

CCSize calcTextSize(const std::string& text, float scale) {
    // TODO - wasteful
    auto* label = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
    label->setScale(scale);
    return label->getScaledContentSize();
}

// Returns the max number of characters we can render from the string
// given max width `maxSize`
size_t findLowerCharacterBound(const std::string& text, float scale, float maxSize) {
    // binary search
    size_t low = 0;
    size_t high = text.length();

    while (low < high) {
        size_t mid = low + (high - low) / 2;
        const std::string& subs = text.substr(0, mid);
        float resultW = calcTextSize(subs, scale).width;
        if (resultW < maxSize) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return low;
}

std::pair<std::string, std::string> splitAt(const std::string& text, size_t length) {
    if (length >= text.length()) {
        return std::make_pair(text, "");
    } else {
        return std::make_pair(text.substr(0, length), text.substr(length));
    }
}

bool ChatMessage::init(const PlayerAccountData& accData, const std::string& message, float lineWidth) {
    // root column layout
    auto* rlayout = ColumnLayout::create()
        ->setAutoScale(false)
        ->setGap(2.f)
        ->setAxisReverse(true)
        ->setCrossAxisLineAlignment(AxisAlignment::Start);

    this->setLayout(rlayout);

    auto* firstLine = CCNode::create();
    firstLine->setID("dankmeme.globed/chat-message-firstline");

    // simpleplayer
    SimplePlayer* sp = SimplePlayer::create(accData.cube);
    sp->updatePlayerFrame(accData.cube, IconType::Cube);
    sp->setColor(GameManager::get()->colorForIdx(accData.color1));
    sp->setSecondColor(GameManager::get()->colorForIdx(accData.color2));
    sp->setGlowOutline(accData.glow);
    sp->setScale(0.25f);
    sp->setID("dankmeme.globed/chat-message-icon");
    sp->setContentSize(sp->m_firstLayer->getContentSize());
    sp->setPosition(sp->getScaledContentSize() - CCSize{0.f, 1.f}); // simpleplayer is silly so anchor point wont work
    firstLine->addChild(sp);

    this->simplePlayer = sp;

    float spWidth = sp->getScale() * sp->m_firstLayer->getContentSize().width;

    auto nameColor = pickNameColor(accData.name);
    auto* nameLabel = CCLabelBMFont::create(fmt::format("[{}]", accData.name).c_str(), "chatFont.fnt");
    nameLabel->setScale(TEXT_SCALE);
    nameLabel->setAnchorPoint({0.f, 0.f});
    nameLabel->setPosition({spWidth + 1.f, 0.f});
    nameLabel->setColor(nameColor);

    this->nameLabel = nameLabel;

    // // the brackets [] should be white (it doesnt work!!!)
    // static_cast<CCSprite*>(nameLabel->getChildByTag(0))->setColor(ccc3(255, 255, 255));
    // static_cast<CCSprite*>(nameLabel->getChildByTag(nameLabel->getChildrenCount() - 1))->setColor(ccc3(255, 255, 255));
    firstLine->addChild(nameLabel);

    float firstLineMsgWidth = lineWidth - spWidth - nameLabel->getScaledContentSize().width;

    size_t splitPos = findLowerCharacterBound(message, TEXT_SCALE, firstLineMsgWidth);
    auto [firstLineMsg, restMsg] = splitAt(message, splitPos);

    auto* firstLineLabel = CCLabelBMFont::create(firstLineMsg.c_str(), "chatFont.fnt");
    firstLineLabel->setScale(TEXT_SCALE);
    firstLineLabel->setAnchorPoint({0.f, 0.f});
    firstLineLabel->setPosition({nameLabel->getPositionX() + nameLabel->getScaledContentSize().width + 1.f, 0.f});
    firstLine->addChild(firstLineLabel);
    firstLine->setContentSize({lineWidth, firstLineLabel->getScaledContentSize().height});
    this->line1 = firstLineLabel;

    this->addChild(firstLine);

    float calcHeight = 0.f;
    calcHeight = firstLineLabel->getScaledContentSize().height;

    // 2nd line
    if (!restMsg.empty()) {
        size_t splitPos = findLowerCharacterBound(restMsg, TEXT_SCALE, lineWidth);
        auto [secondLineMsg, restMsg2] = splitAt(restMsg, splitPos);

        auto* restLabel = CCLabelBMFont::create(secondLineMsg.c_str(), "chatFont.fnt");
        restLabel->setScale(TEXT_SCALE);
        auto lblSize = restLabel->getScaledContentSize();
        this->addChild(restLabel);
        calcHeight += 2.f + lblSize.height;
        this->line2 = restLabel;

        // 3rd line god forbid
        if (!restMsg2.empty()) {
            auto* restLabelExtra = CCLabelBMFont::create(restMsg2.c_str(), "chatFont.fnt");
            restLabelExtra->setScale(TEXT_SCALE);
            auto lblSize = restLabelExtra->getScaledContentSize();
            this->addChild(restLabelExtra);
            calcHeight += 2.f + lblSize.height;
            this->line3 = restLabelExtra;
        }
    }

    this->setContentSize({lineWidth, calcHeight});
    this->updateLayout();

    return true;
}

void ChatMessage::setOpacity(GLubyte opacity) {
    if (simplePlayer) {
        simplePlayer->m_firstLayer->setOpacity(opacity);
        simplePlayer->m_secondLayer->setOpacity(opacity);
    }

    if (line1) {
        line1->setOpacity(opacity);
    }

    if (line2) {
        line2->setOpacity(opacity);
    }

    if (line3) {
        line3->setOpacity(opacity);
    }
}

ChatMessage* ChatMessage::create(const PlayerAccountData& accData, const std::string& message, float lineWidth) {
    auto ret = new ChatMessage;
    if (ret && ret->init(accData, message, lineWidth)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}