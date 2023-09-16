#include "ppa_interp.hpp"

// this code sucks, and sucks a lot
void InterpolationPPAEngine::updateSpecificPlayer(
    CCSprite* player,
    const SpecificIconData& data,
    float frameDelta,
    int playerId,
    bool isSecond
) {
    auto lastFrameMap = isSecond ? &lastFrameP2 : &lastFrameP1;
    auto preLastFrameMap = isSecond ? &preLastFrameP2 : &preLastFrameP1;

    auto pos = CCPoint{data.x, data.y};
    auto rot = CCPoint{data.xRot, data.yRot};

    if (!lastFrameMap->contains(playerId)) {
        lastFrameMap->insert(std::make_pair(playerId, std::make_pair(CCPoint{-1.f, -1.f}, CCPoint{0.f, 0.f})));
        preLastFrameMap->insert(std::make_pair(playerId, std::make_pair(CCPoint{0.f, 0.f}, CCPoint{0.f, 0.f})));
        return;
    }

    auto lastFrameData = lastFrameMap->at(playerId);
    auto lastPos = lastFrameData.first;
    auto lastRot = lastFrameData.second;

    auto preLastFrameData = preLastFrameMap->at(playerId);
    auto preLastPos = preLastFrameData.first;
    auto preLastRot = preLastFrameData.second;

    bool realFrame = pos != preLastPos || rot != preLastRot;

    // if real frame, change lastFrame to the frame we just received
    // and update the player with the old frame
    if (realFrame) {
        player->setPosition(preLastPos);
        player->setRotationX(preLastRot.x);
        player->setRotationY(preLastRot.y);
        (*lastFrameMap)[playerId] = std::make_pair(preLastPos, preLastRot);
        (*preLastFrameMap)[playerId] = std::make_pair(pos, rot);
        return;
    }

    // if fake frame, do interpolation

    float deltaRatio = targetUpdateDelay / frameDelta;
    auto posDelta = preLastPos - lastPos;
    auto rotDelta = preLastRot - lastRot;

    // log::debug("posDelta: {}, rotDelta: {}, deltaRatio: {}", posDelta, rotDelta, deltaRatio);

    auto iPos = player->getPosition() + (posDelta / deltaRatio);
    auto iRotX = player->getRotationX() + (rotDelta.x / deltaRatio);
    auto iRotY = player->getRotationY() + (rotDelta.y / deltaRatio);

    log::debug("iDelta: {}", posDelta / deltaRatio);

    player->setPosition(iPos);
    player->setRotationX(iRotX);
    player->setRotationY(iRotY);
}

void InterpolationPPAEngine::addPlayer(int playerId, const PlayerData& data) {}

void InterpolationPPAEngine::removePlayer(int playerId) {
    lastFrameP1.erase(playerId);
    lastFrameP2.erase(playerId);
}