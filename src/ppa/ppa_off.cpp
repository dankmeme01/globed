#include "ppa_off.hpp"

void DisabledPPAEngine::updateSpecificPlayer(
    RemotePlayer* player,
    const SpecificIconData& data,
    float frameDelta,
    int playerId,
    bool isSecond
) {
    player->setPosition({data.x, data.y});
    player->setRotation(data.rot);
}