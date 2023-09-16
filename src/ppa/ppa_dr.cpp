#include "ppa_dr.hpp"

// Rotation is broken unfortunately
void DRPPAEngine::updateSpecificPlayer(
    CCSprite* player,
    const SpecificIconData& data,
    float frameDelta,
    int playerId,
    bool isSecond
) {
    auto newPosition = CCPoint{data.x, data.y};
    CCPoint oldPosition;
    
    if (isSecond)
        oldPosition = lastRealPos[playerId].second;
    else
        oldPosition = lastRealPos[playerId].first;

    auto newRotation = CCPoint{data.xRot, data.yRot};
    // auto oldRotation = lastRealRot[playerId];

    bool realFrame = oldPosition != newPosition /* || oldRotation != newRotation */;

    if (realFrame) {
        if (isSecond)
            errCorrections[playerId].second = newPosition - oldPosition;
        else
            errCorrections[playerId].first = newPosition - oldPosition;

        // rotCorrections[playerId] = newRotation - oldRotation;
        if (isSecond)
            lastRealPos[playerId].second = newPosition;
        else
            lastRealPos[playerId].first = newPosition;
        // lastRealRot[playerId] = newRotation;
        player->setPosition(newPosition);
        player->setRotationX(newRotation.x);
        player->setRotationY(newRotation.y);
    } else {
        float deltaMult = targetUpdateDelay / frameDelta;
        CCPoint errc;
        if (isSecond)
            errc = errCorrections[playerId].second;
        else
            errc = errCorrections[playerId].first;

        auto pos = player->getPosition() + errc / deltaMult;
        player->setPosition(pos);
        // auto rx = player->getRotationX() + rotCorrections[playerId].x / deltaMult;
        // auto ry = player->getRotationY() + rotCorrections[playerId].y / deltaMult;
        // player->setRotationX(rx);
        // player->setRotationY(ry);
    }
}

void DRPPAEngine::addPlayer(int playerId, const PlayerData& data) {
    errCorrections.insert(std::make_pair(playerId, std::make_pair(CCPoint{0.f, 0.f}, CCPoint{0.f, 0.f})));
    // rotCorrections.insert(std::make_pair(playerId, CCPoint{0.f, 0.f}));
    lastRealPos.insert(std::make_pair(playerId, std::make_pair(CCPoint{0.f, 0.f}, CCPoint{0.f, 0.f})));
    // lastRealRot.insert(std::make_pair(playerId, CCPoint{0.f, 0.f}));
}

void DRPPAEngine::removePlayer(int playerId) {
    errCorrections.erase(playerId);
    // rotCorrections.erase(playerId);
    lastRealPos.erase(playerId);
    // lastRealRot.erase(playerId);
}