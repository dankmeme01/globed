#pragma once

struct TextMessage {
    int sender;
    std::string message;
    cocos2d::ccColor3B color; //ignored for non server messages (aka when the id isn't -1)
};