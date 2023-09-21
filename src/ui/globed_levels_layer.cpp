#include "globed_levels_layer.hpp"
#include "globed_levels_listview.hpp"

bool GlobedLevelsLayer::init() {
    if (!CCLayer::init()) return false;
    auto windowSize = CCDirector::get()->getWinSize();

    globed_util::ui::addBackground(this);

    auto menu = CCMenu::create();
    globed_util::ui::addBackButton(this, menu, menu_selector(GlobedLevelsLayer::goBack));
    this->addChild(menu);

    globed_util::ui::enableKeyboard(this);

    // layout for da buttons
    auto buttonsLayout = ColumnLayout::create();
    auto buttonsMenu = CCMenu::create();
    buttonsMenu->setLayout(buttonsLayout);
    buttonsLayout->setAxisAlignment(AxisAlignment::Start);
    buttonsLayout->setGap(5.0f);

    buttonsMenu->setPosition({15.f, 15.f});
    buttonsMenu->setAnchorPoint({0.f, 0.f});

    this->addChild(buttonsMenu);
    
    // add a button for refreshing
    auto refreshSprite = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    auto refreshButton = CCMenuItemSpriteExtra::create(refreshSprite, this, menu_selector(GlobedLevelsLayer::onRefreshButton));
    buttonsMenu->addChild(refreshButton);

    buttonsMenu->updateLayout();

    onRefreshButton(nullptr);

    scheduleUpdate();

    return true;
}

void GlobedLevelsLayer::update(float dt) {
    if (!g_levelsLoading && !m_fetchingLevels && m_stillLoading) {
        fetchLevelList();
    }
}

// fetchLevelList is called after sending NMRequestLevelList AND receiving LevelListResponse from the server.
void GlobedLevelsLayer::fetchLevelList() {
    m_toDownload = globed_util::mapKeys(*g_levelsList.lock());
    m_fetchingLevels = true;
    log::debug("Downloading {} levels", m_toDownload.size());

    // show loading circle
    
    m_loadingCircle = LoadingCircle::create();
    m_loadingCircle->setParentLayer(this);
    m_loadingCircle->setPosition({0.f, 0.f});
    m_loadingCircle->show();

    nextLevel();
}

void GlobedLevelsLayer::updateLevelList() {
    if (m_list) m_list->removeFromParent();
    auto windowSize = CCDirector::get()->getWinSize();

    auto list = GlobedLevelsListView::create(createLevelList(), LIST_WIDTH, LIST_HEIGHT);
    m_list = GJListLayer::create(list, LIST_TITLE, LIST_COLOR, LIST_WIDTH, LIST_HEIGHT);
    m_list->setPosition(windowSize / 2 - m_list ->getScaledContentSize() / 2);
    this->addChild(m_list);
}

CCArray* GlobedLevelsLayer::createLevelList() {
    auto cells = CCArray::create();
    std::vector<GJGameLevel*> sortedLevels;

    auto levelCache = g_levelDataCache.lock();
    auto levelList = g_levelsList.lock();

    for (int levelId : globed_util::mapKeys(*levelList)) {
        if (levelCache->contains(levelId)) {
            sortedLevels.push_back(levelCache->at(levelId));
        }
    }

    // this sorts the levels by their player count (descending)
    auto comparator = [&levelList](GJGameLevel* a, GJGameLevel* b) {
        if (!levelList->contains(a->m_levelID) || !levelList->contains(b->m_levelID)) {
            return true;
        }

        auto aVal = levelList->at(a->m_levelID);
        auto bVal = levelList->at(b->m_levelID);

        return aVal > bVal;
    };

    std::sort(sortedLevels.begin(), sortedLevels.end(), comparator);

    for (GJGameLevel* level : sortedLevels) {
        cells->addObject(level);
    }

    return cells;
}

void GlobedLevelsLayer::onRefreshButton(CCObject* sender) {
    if (g_levelsLoading || m_fetchingLevels) return;

    m_stillLoading = true;
    g_levelsLoading = true;
    g_netMsgQueue.push(NMRequestLevelList {});

    // create an empty list

    if (m_list) m_list->removeFromParent();

    auto windowSize = CCDirector::get()->getWinSize();

    auto list = GlobedLevelsListView::create(CCArray::create(), LIST_WIDTH, LIST_HEIGHT);
    m_list = GJListLayer::create(list, LIST_TITLE, LIST_COLOR, LIST_WIDTH, LIST_HEIGHT);
    m_list->setPosition(windowSize / 2 - m_list ->getScaledContentSize() / 2);
    this->addChild(m_list);
}

GlobedLevelsLayer::~GlobedLevelsLayer() {
    g_levelsLoading = false;
    if (m_loadingCircle) {
        m_loadingCircle->fadeAndRemove();
        m_loadingCircle = nullptr;
    }
    GameLevelManager::get()->m_levelDownloadDelegate = nullptr;
}

// level downloading itself

void GlobedLevelsLayer::nextLevel() {
    int fetchId = -69;

    while (!m_toDownload.empty()) {
        int id = m_toDownload.back();
        m_toDownload.pop_back();

        // skip if cached, otherwise try fetch
        if (!g_levelDataCache.lock()->contains(id)) {
            fetchId = id;
            break;
        }
    }

    if (fetchId == -69) {
        m_fetchingLevels = false;
        if (m_loadingCircle) {
            m_loadingCircle->fadeAndRemove();
            m_loadingCircle = nullptr;
        }
        m_stillLoading = false;
        updateLevelList();
    } else {
        GameLevelManager::get()->m_levelDownloadDelegate = this;
        log::debug("downloading level: {}", fetchId);
        GameLevelManager::get()->downloadLevel(fetchId, false);
    }
}

void GlobedLevelsLayer::levelDownloadFinished(GJGameLevel* level) {
    level->m_gauntletLevel = false;
    level->m_gauntletLevel2 = false;
    log::debug("finished downloading level: {}", level->m_levelID.value());
    level->retain();
    (*g_levelDataCache.lock())[level->m_levelID.value()] = level;

    nextLevel();
}

void GlobedLevelsLayer::levelDownloadFailed(int levelID) {
    log::warn("Failed to download level {}", levelID);
    nextLevel();
}

DEFAULT_GOBACK_DEF(GlobedLevelsLayer)
DEFAULT_KEYDOWN_DEF(GlobedLevelsLayer)
DEFAULT_CREATE_DEF(GlobedLevelsLayer)