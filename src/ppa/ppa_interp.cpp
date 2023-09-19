#include "ppa_interp.hpp"

// this code sucks, and sucks a lot
void InterpolationPPAEngine::updateSpecificPlayer(
    RemotePlayer* player,
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

    float* preservedDashDelta = isSecond ? &preservedDashDeltaP2 : &preservedDashDeltaP1;

    if (realFrame) {
        player->setPosition(preLastPos);
        
        if (data.isDashing && (data.gameMode == IconGameMode::CUBE || data.gameMode == IconGameMode::BALL)) {
            float dashDelta = DASH_DEGREES_PER_SECOND * targetUpdateDelay;
            *preservedDashDelta = std::fmod(*preservedDashDelta + dashDelta, 360.f);
            player->setRotationX(preLastRot.x + (data.isUpsideDown ? *preservedDashDelta * -1 : *preservedDashDelta));
            player->setRotationY(preLastRot.y + (data.isUpsideDown ? *preservedDashDelta * -1 : *preservedDashDelta));
        } else {
            *preservedDashDelta = 0.f;
            player->setRotationX(preLastRot.x);
            player->setRotationY(preLastRot.y);
        }

        (*lastFrameMap)[playerId] = std::make_pair(preLastPos, preLastRot);
        (*preLastFrameMap)[playerId] = std::make_pair(pos, rot);

        return;
    }

    // if fake frame, do interpolation

    float deltaRatio = targetUpdateDelay / frameDelta;
    auto posDelta = preLastPos - lastPos;
    auto rotDelta = preLastRot - lastRot;

    // disable rot interp if the spin is too big (such as from 580 degrees to -180)
    if (abs(rotDelta.x) > 360) {
        rotDelta.x = 0;
        rotDelta.y = 0;
    }

    if (data.isDashing && (data.gameMode == IconGameMode::CUBE || data.gameMode == IconGameMode::BALL)) {
        // dash = 720 degrees per second
        float dashDelta = DASH_DEGREES_PER_SECOND * targetUpdateDelay;
        if (data.isUpsideDown) {
            dashDelta *= -1; // dash in reverse direction
        }
        rotDelta.x = dashDelta;
        rotDelta.y = dashDelta;
    }

    // disable Y interpolation for spider, so it doesn't appear mid-air
    if (data.gameMode == IconGameMode::SPIDER && data.isGrounded) {
        posDelta.y = 0;
    }

    float iRotX, iRotY;
    auto iPos = player->getPosition() + (posDelta / deltaRatio);

    iRotX = player->getRotationX() + (rotDelta.x / deltaRatio);
    iRotY = player->getRotationY() + (rotDelta.y / deltaRatio);

    player->setPosition(iPos);
    player->setRotationX(iRotX);
    player->setRotationY(iRotY);
}

void InterpolationPPAEngine::addPlayer(int playerId, const PlayerData& data) {}

void InterpolationPPAEngine::removePlayer(int playerId) {
    lastFrameP1.erase(playerId);
    lastFrameP2.erase(playerId);
}