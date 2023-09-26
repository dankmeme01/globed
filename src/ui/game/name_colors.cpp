#include "name_colors.hpp"

cocos2d::ccColor3B pickNameColor(std::string name) {
    // convert to lowercase
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    if (name == "dankmeme01") {
        return {.r = 204, .g = 52, .b = 235};
    } else if (name == "robtop") {
        return {.r = 235, .g = 222, .b = 52};
    } else if (name == "ca7x3") {
        return {.r = 255, .g = 102, .b = 204};
    }
    return {255, 255, 255};
}