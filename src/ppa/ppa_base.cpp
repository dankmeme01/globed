#include "ppa_base.hpp"

void PPAEngine::updatePlayer(
    std::pair<RemotePlayer*, RemotePlayer*>& player,
    const PlayerData& data,
    float frameDelta,
    int playerId
) {
    if (data.player1.isHidden) {
        player.first->setVisible(false);
        updateHiddenPlayer(player.first, data.player1, frameDelta, playerId, false);
    } else {
        player.first->setVisible(true);
        auto scale = data.player1.isMini ? 0.6f : 1.0f;
        player.first->setScale(scale);
        player.first->setScaleY(scale * (data.player1.isUpsideDown ? -1 : 1));
        // player.first->setScaleY(abs(player.first->getScaleY()) * (data.player1.isUpsideDown ? -1 : 1));
        player.first->tick(data.player1.gameMode);
        updateSpecificPlayer(player.first, data.player1, frameDelta, playerId, false);
    }

    if (data.player2.isHidden) {
        player.second->setVisible(false);
        updateHiddenPlayer(player.second, data.player2, frameDelta, playerId, true);
    } else {
        player.second->setVisible(true);
        auto scale = data.player2.isMini ? 0.6f : 1.0f;
        player.second->setScale(scale);
        player.second->setScaleY(scale * (data.player2.isUpsideDown ? -1 : 1));
        // player.second->setScaleY(abs(player.second->getScaleY()) * (data.player2.isUpsideDown ? -1 : 1));
        player.second->tick(data.player2.gameMode);
        updateSpecificPlayer(player.second, data.player2, frameDelta, playerId, true);
    }
}

void PPAEngine::updateHiddenPlayer(
    RemotePlayer* player,
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