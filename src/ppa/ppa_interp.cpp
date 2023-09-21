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
    auto rot = data.rot;
    
    if (!lastFrameMap->contains(playerId)) {
        lastFrameMap->insert(std::make_pair(playerId, std::make_pair(CCPoint{-1.f, -1.f}, 0.f)));
        preLastFrameMap->insert(std::make_pair(playerId, std::make_pair(CCPoint{0.f, 0.f}, 0.f)));
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
            player->setRotation(preLastRot + (data.isUpsideDown ? *preservedDashDelta * -1 : *preservedDashDelta));
        } else {
            *preservedDashDelta = 0.f;
            player->setRotationX(preLastRot);
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
    if (abs(rotDelta) > 360) {
        rotDelta = 0;
    }

    bool hasDashDelta = false;
    if (data.isDashing && (data.gameMode == IconGameMode::CUBE || data.gameMode == IconGameMode::BALL)) {
        // dash = 720 degrees per second
        float dashDelta = DASH_DEGREES_PER_SECOND * targetUpdateDelay;
        if (data.isUpsideDown) {
            dashDelta *= -1; // dash in reverse direction
        }
        rotDelta = dashDelta;
        rotDelta = dashDelta;
        hasDashDelta = true;
    }

    // disable Y interpolation for spider, so it doesn't appear mid-air
    if (data.gameMode == IconGameMode::SPIDER && data.isGrounded) {
        posDelta.y = 0;
    }

    float iRotX, iRotY;
    auto iPos = player->getPosition() + (posDelta / deltaRatio);

    iRotX = player->getRotation() + (rotDelta / deltaRatio);
    
    auto wholePosDelta = iPos - lastPos;
    auto wholeRotDelta = iRotX - lastRot;

    // packet loss can cause extrapolation, remove it.
    // this isn't exactly bad normally, but when you die, you go through blocks without these conditions.
    if (abs(wholePosDelta.x) <= abs(posDelta.x)) {
        player->setPosition(iPos);
    }

    if (hasDashDelta || abs(wholeRotDelta) <= abs(rotDelta)) {
        player->setRotationX(iRotX);
        player->setRotationY(iRotY);
    }
}

void InterpolationPPAEngine::addPlayer(int playerId, const PlayerData& data) {
    // moving this from the function above here causes a hang, idk why
    // lastFrameP1.insert(std::make_pair(playerId, std::make_pair(CCPoint{-1.f, -1.f}, CCPoint{0.f, 0.f})));
    // lastFrameP2.insert(std::make_pair(playerId, std::make_pair(CCPoint{0.f, 0.f}, CCPoint{0.f, 0.f})));
}

void InterpolationPPAEngine::removePlayer(int playerId) {
    lastFrameP1.erase(playerId);
    lastFrameP2.erase(playerId);
    preLastFrameP1.erase(playerId);
    preLastFrameP2.erase(playerId);
}