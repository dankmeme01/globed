#include "correction.hpp"

const float DASH_DEGREES_PER_SECOND = 900.f; // this is weird, if too fast use 720.f

void PlayerCorrection::updateSpecificPlayer(
    RemotePlayer* player,
    const SpecificIconData& data,
    float frameDelta,
    int playerId,
    bool isSecond,
    bool isPractice,
    float timestamp
) {
    if (!data.isHidden) {
        player->setVisible(true);
        auto scale = data.isMini ? 0.6f : 1.0f;
        player->setScale(scale);

        if (data.gameMode != IconGameMode::CUBE) { // cube reacts differently to m_isUpsideDown i think
            player->setScaleY(scale * (data.isUpsideDown ? -1 : 1));
        }
        
        player->tick(data, isPractice);
    } else {
        player->setVisible(false);
        return;
    }

    auto& cDataAll = playerData.at(playerId);
    SpecificCorrectionData* cData = isSecond ? &cDataAll.p2 : &cDataAll.p1;

    auto currentPos = CCPoint{data.x, data.y};
    auto currentRot = data.rot;

    // prevNewerPos is the previous real frame
    // prevOlderPos is the one before that
    auto prevNewerPos = cData->newerFrame.pos;
    auto prevNewerRot = cData->newerFrame.rot;

    auto prevOlderPos = cData->olderFrame.pos;
    auto prevOlderRot = cData->olderFrame.rot;

    bool realFrame = currentPos != prevNewerPos || currentRot != prevNewerRot;

    if (realFrame) {
        PCFrameInfo displayedFrame {
            .pos = cData->newerFrame.pos,
            .timestamp = cData->newerFrame.timestamp
        };

        if (data.isDashing && (data.gameMode == IconGameMode::CUBE || data.gameMode == IconGameMode::BALL)) {
            float dashDelta = DASH_DEGREES_PER_SECOND * targetUpdateDelay;
            cData->preservedDashDelta = std::fmod(cData->preservedDashDelta + dashDelta, 360.f);
            displayedFrame.rot = cData->newerFrame.rot + (data.isUpsideDown ? (cData->preservedDashDelta * -1) : cData->preservedDashDelta);
        } else {
            cData->preservedDashDelta = 0.f;
            displayedFrame.rot = cData->newerFrame.rot;
        }

        cData->olderFrame = cData->newerFrame;
        cData->newerFrame = PCFrameInfo { currentPos, currentRot, timestamp};

        applyFrame(player, displayedFrame);
        cDataAll.timestamp = cData->olderFrame.timestamp;
        return;
    }
    
    // interpolate
    float oldTime = cData->olderFrame.timestamp;
    float newTime = cData->newerFrame.timestamp;
    float* currentTime = &cDataAll.timestamp;

    float wholeTimeDelta = newTime - oldTime; // time between 2 packets
    float timeDelta = newTime - *currentTime;
    float timeDeltaRatio = 1.f - timeDelta / wholeTimeDelta;
    
    if (timeDeltaRatio >= 1.0f) {
        // extrapolate (unimpl)
        return;
    }

    auto pos = CCPoint{
        std::lerp(cData->olderFrame.pos.x, cData->newerFrame.pos.x, timeDeltaRatio),
        std::lerp(cData->olderFrame.pos.y, cData->newerFrame.pos.y, timeDeltaRatio),
    };

    // disable Y interpolation for spider, so it doesn't appear mid-air
    if (data.gameMode == IconGameMode::SPIDER && data.isGrounded) {
        pos.y = cData->olderFrame.pos.y;
    }

    float rot = cData->newerFrame.rot;

    // if dashing, make our own rot interpolation with preservedDashDelta
    if (data.isDashing && (data.gameMode == IconGameMode::CUBE || data.gameMode == IconGameMode::BALL)) {
        float dashDelta = DASH_DEGREES_PER_SECOND * targetUpdateDelay;
        if (data.isUpsideDown) {
            dashDelta *= -1;
        }

        auto base = player->getRotation();

        rot = base + std::lerp(0, dashDelta, timeDeltaRatio);
    } else {
        // disable rot interp if the spin is too big (such as from 580 degrees to -180).
        // dont ask me why that can happen, just gd moment.
        if (abs(cData->newerFrame.rot - cData->olderFrame.rot) < 360) {
            rot = std::lerp(cData->olderFrame.rot, cData->newerFrame.rot, timeDeltaRatio);
        }
    }

    auto frame = PCFrameInfo {
        .pos = pos,
        .rot = rot,
        .timestamp = *currentTime,
    };

    applyFrame(player, frame);
}

void PlayerCorrection::updatePlayer(
    std::pair<RemotePlayer*, RemotePlayer*>& player,
    const PlayerData& data,
    float frameDelta,
    int playerId
    // uint64_t timestamp
) {
    updateSpecificPlayer(player.first, data.player1, frameDelta, playerId, false, data.isPractice, data.timestamp);
    updateSpecificPlayer(player.second, data.player2, frameDelta, playerId, true, data.isPractice, data.timestamp);

    playerData.at(playerId).timestamp = playerData.at(playerId).timestamp + frameDelta;
}

void PlayerCorrection::applyFrame(RemotePlayer* player, const PCFrameInfo& data) {
    player->setPosition(data.pos);
    player->setRotation(data.rot);
}

void PlayerCorrection::addPlayer(int playerId, const PlayerData& data) {
    auto pcData = PlayerCorrectionData {
        data.timestamp,
        SpecificCorrectionData {
            0.f,
            PCFrameInfo {CCPoint{0.f, 0.f}, 0.f, data.timestamp},
            PCFrameInfo {CCPoint{0.f, 0.f}, 0.f, data.timestamp},
        },
        SpecificCorrectionData {
            0.f,
            PCFrameInfo {CCPoint{0.f, 0.f}, 0.f, data.timestamp},
            PCFrameInfo {CCPoint{0.f, 0.f}, 0.f, data.timestamp},
        }
    };
    playerData.insert(std::make_pair(playerId, pcData));
}
void PlayerCorrection::removePlayer(int playerId) {
    playerData.erase(playerId);
}
void PlayerCorrection::setTargetDelta(float dt) {
    targetUpdateDelay = dt;
}