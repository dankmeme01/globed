#include "name_colors.hpp"

cocos2d::ccColor3B pickNameColor(const char* name) {
    if (std::strcmp(name, "dankmeme01") == 0) {
        return {.r = 204, .g = 52, .b = 235};
    } else if (std::strcmp(name, "RobTop") == 0) {
        return {.r = 235, .g = 222, .b = 52};
    }
    return {255, 255, 255};
}