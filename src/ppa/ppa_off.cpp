#include "ppa_off.hpp"

void DisabledPPAEngine::updateSpecificPlayer(
    RemotePlayer* player,
    const SpecificIconData& data,
    float frameDelta,
    int playerId,
    bool isSecond
) {
    player->setPosition({data.x, data.y});
    player->setRotationX(data.xRot);
    player->setRotationY(data.yRot);
}