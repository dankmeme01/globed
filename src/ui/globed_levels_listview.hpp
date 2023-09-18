// thank you cvolton
// https://github.com/Cvolton/betterinfo-geode/blob/master/src/layers/LevelSearchListView.h

#pragma once
#include <Geode/Bindings.hpp>
#include "globed_list_view.hpp"

class GlobedLevelsListView : public GlobedListView {
protected:
    void setupList() override;
    TableViewCell* getListCell(const char* key) override;
    void loadCell(TableViewCell* cell, int index) override;
public:
    static GlobedLevelsListView* create(cocos2d::CCArray* levels, float width, float height);
};