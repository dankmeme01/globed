#include "spectate_popup.hpp"
#include "globed_list_view.hpp"
#include "spectate_user_cell.hpp"
#include "../global_data.hpp"
#include "../util.hpp"

bool SpectatePopup::setup() {
    auto winSize = CCDirector::get()->getWinSize();
    setTitle("Spectate a player");
    auto players = g_pCorrector.getPlayerIds();
    auto dCache = g_accDataCache.lock();

    auto cells = CCArray::create();

    for (int id : players) {
        if (!dCache->contains(id)) continue;
        auto accData = dCache->at(id);
        
        auto player = SimplePlayer::create(accData.cube);
        player->updatePlayerFrame(accData.cube, IconType::Cube);
        player->setColor(GameManager::get()->colorForIdx(accData.color1));
        player->setSecondColor(GameManager::get()->colorForIdx(accData.color1));

        auto cell = SpectateUserCell::create({SPP_LIST_SIZE.width, SPP_CELL_HEIGHT}, accData.name, player);
        cells->addObject(cell);
    }

    auto listview = ListView::create(cells, SPP_CELL_HEIGHT, SPP_LIST_SIZE.width, SPP_LIST_SIZE.height);
    m_list = GJCommentListLayer::create(listview, "Spectate", {192, 114, 62, 255}, SPP_LIST_SIZE.width, SPP_LIST_SIZE.height, false);
    m_list->setPosition({m_size.width / 4, 70.f});
    m_mainLayer->addChild(m_list);

    return true;
}

SpectatePopup* SpectatePopup::create() {
    auto ret = new SpectatePopup;
    if (ret && ret->init(420.f, 240.f)) {
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);
    return nullptr;
}