// thank you cvolton
// https://github.com/Cvolton/betterinfo-geode/blob/master/src/layers/CvoltonListView.h

#pragma once

#include <Geode/Geode.hpp>

/**
 * @brief      This class only exists as a bugfix to CustomListView
 */
class GlobedListView : public CustomListView {
protected:
    bool init(cocos2d::CCArray* entries, int btype, float width, float height);
    virtual ~GlobedListView();
};