#include "globed_levels_listview.hpp"
#include "globed_level_cell.hpp"
#include "../global_data.hpp"

GlobedLevelsListView* GlobedLevelsListView::create(cocos2d::CCArray *levels, float width, float height) {
    auto ret = new GlobedLevelsListView;
    if (ret && ret->init(levels, 0x420, width, height)) {
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);
    return nullptr;
}

void GlobedLevelsListView::setupList() {
    this->m_itemSeparation = 90.0f;

    this->m_tableView->reloadData();

    auto coverage = geode::cocos::calculateNodeCoverage(m_tableView->m_contentLayer->getChildren());
    if (this->m_entries->count() > 4)
        m_tableView->m_contentLayer->setContentSize({-coverage.origin.x + coverage.size.width, -coverage.origin.y + coverage.size.height});

    this->m_tableView->moveToTop();

    if (this->m_entries->count() == 1)
        this->m_tableView->moveToTopWithOffset(this->m_itemSeparation);
}

void GlobedLevelsListView::loadCell(TableViewCell* cell, int index) {
    auto levelObject = static_cast<GJGameLevel*>(this->m_entries->objectAtIndex(index));
    auto levelId = levelObject->m_levelID.value();
    static_cast<GlobedLevelCell*>(cell)->loadFromLevel(levelObject);

    unsigned short playerCount = 0;
    auto levelCache = g_levelsList.lock();
    if (levelCache->contains(levelId)) {
        playerCount = levelCache->at(levelId);
    }
    static_cast<GlobedLevelCell*>(cell)->updatePlayerCount(playerCount);
    static_cast<GlobedLevelCell*>(cell)->updateBGColor(index);
}

TableViewCell* GlobedLevelsListView::getListCell(const char* key) {
    return new GlobedLevelCell(key, this->m_width, this->m_itemSeparation);
}