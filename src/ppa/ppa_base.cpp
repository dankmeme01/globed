#include "ppa_base.hpp"

void PPAEngine::updatePlayer(
    std::pair<CCSprite*, CCSprite*>& player,
    const PlayerData& data,
    float frameDelta,
    int playerId
) {
    if (data.player1.isHidden) {
        player.first->setVisible(false);
        updateHiddenPlayer(player.first, data.player1, frameDelta, playerId, false);
    } else {
        player.first->setVisible(true);
        player.first->setScaleY(abs(player.first->getScaleY()) * (data.player1.isUpsideDown ? -1 : 1));
        updateSpecificPlayer(player.first, data.player1, frameDelta, playerId, false);
    }

    if (data.player2.isHidden) {
        player.second->setVisible(false);
        updateHiddenPlayer(player.second, data.player2, frameDelta, playerId, true);
    } else {
        player.second->setVisible(true);
        player.second->setScaleY(abs(player.second->getScaleY()) * (data.player2.isUpsideDown ? -1 : 1));
        updateSpecificPlayer(player.second, data.player2, frameDelta, playerId, true);
    }
}

void PPAEngine::updateHiddenPlayer(
    CCSprite* player,
    const SpecificIconData& data,
    float frameDelta,
    int playerId,
    bool isSecond
) {}

void PPAEngine::addPlayer(int playerId, const PlayerData& data) {}
void PPAEngine::removePlayer(int playerId) {}

void PPAEngine::setTargetDelta(float dt) {
    targetUpdateDelay = dt;
}

PPAEngine::~PPAEngine() {}