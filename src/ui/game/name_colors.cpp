#include "name_colors.hpp"

using namespace geode::prelude;

cocos2d::ccColor3B pickNameColor(std::string name) {
    // convert to lowercase
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    if (name == "dankmeme01") {
        return ccc3(204, 52, 235);
    } else if (name == "robtop") {
        return ccc3(235, 222, 52);
    } else if (name == "ca7x3") {
        return ccc3(255, 102, 204);
    } else if (name == "croozington") {
        return ccc3(185, 0, 255);
    }
    return ccc3(255, 255, 255);
}