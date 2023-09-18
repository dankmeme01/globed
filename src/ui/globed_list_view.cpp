// thank you cvolton

#include "globed_list_view.hpp"

using namespace gd;
using namespace cocos2d;

bool GlobedListView::init(cocos2d::CCArray* entries, int btype, float width, float height) {
    if(!CustomListView::init(entries, (BoomListType) btype, width, height)) return false;

    m_tableView->retain();
    return true;
}

GlobedListView::~GlobedListView() {
    m_tableView->release();
}