#include <Geode/Geode.hpp>
#include <chrono>
#include <map>
#include <vector>

#include "../util.hpp"
#include "server_list_cell.hpp"

using namespace geode::prelude;
constexpr float CELL_HEIGHT = 45.0f;
const CCSize LIST_SIZE = { 358.f, 190.f };

class GlobedMenuLayer : public CCLayer {
protected:
    GJListLayer* m_list = nullptr;
    std::vector<GameServer> m_internalServers;
    std::chrono::system_clock::time_point m_lastPing;
    std::chrono::system_clock::time_point m_lastRefresh;

    bool init();
    void refreshServers();
    void sendMessage(Message msg);

    CCArray* createServerList();
    void checkErrorsAndPing(float unused);

public:
    DEFAULT_GOBACK
    DEFAULT_KEYDOWN
    DEFAULT_CREATE(GlobedMenuLayer)
};