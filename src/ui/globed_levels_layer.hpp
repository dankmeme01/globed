#pragma once
#include <Geode/Geode.hpp>
#include "../util.hpp"

const char* const LIST_TITLE = "Server levels";
const float LIST_WIDTH = 356.f;
const float LIST_HEIGHT = 220.f;
const ccColor4B LIST_COLOR = {191, 114, 62, 255};

class GlobedLevelsLayer : public CCLayer, LevelDownloadDelegate {
protected:
    GJListLayer* m_list = nullptr;
    LoadingCircle* m_loadingCircle;
    std::vector<int> m_toDownload;
    bool m_fetchingLevels, m_stillLoading = false;

    bool init();
    void update(float dt);

    void fetchLevelList();
    void updateLevelList();
    CCArray* createLevelList();

    void onRefreshButton(CCObject* sender);

    void nextLevel();
    void levelDownloadFinished(GJGameLevel* level);
    void levelDownloadFailed(int levelId);

    ~GlobedLevelsLayer();
public:
    DEFAULT_GOBACK
    DEFAULT_KEYDOWN
    DEFAULT_CREATE(GlobedLevelsLayer)
};