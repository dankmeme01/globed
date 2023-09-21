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
        
        // player.first->setScaleY(abs(player.first->getScaleY()) * (data.player1.isUpsideDown ? -1 : 1));
        player->tick(data, isPractice);
    } else {
        player->setVisible(false);
        return;
    }

    auto& cDataAll = playerData.at(playerId);
    SpecificCorrectionData* cData = isSecond ? &cDataAll.p2 : &cDataAll.p1;

    auto currentPos = CCPoint{data.x, data.y};
    auto currentRot = data.rot;
    // log::debug("current pos: s{},{}; rot: {}", currentPos.x, currentPos.y, currentRot);

    // prevNewerPos is the previous real frame
    // prevOlderPos is the one before that
    auto prevNewerPos = cData->newerFrame.pos;
    auto prevNewerRot = cData->newerFrame.rot;

    auto prevOlderPos = cData->olderFrame.pos;
    auto prevOlderRot = cData->olderFrame.rot;

    bool realFrame = currentPos != prevNewerPos || currentRot != prevNewerRot;

    // log::debug("!! timestamps: now: {}, new: {}, old: {}", timestamp, cData->newerFrame.timestamp, cData->olderFrame.timestamp);
    if (realFrame) {
        cData->olderFrame = cData->newerFrame;
        cData->newerFrame = PCFrameInfo { currentPos, currentRot, timestamp};

        // log::debug("real frame");
        // log::debug("applying real frame: {},{}; rot: {}", cData->olderFrame.pos.x, cData->olderFrame.pos.y, cData->olderFrame.rot);
        applyFrame(player, cData->olderFrame);
        // log::debug("real {}", player->getPositionX());
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

    // log::debug("whole timeDelta: {} - {} = {}", newTime, oldTime, wholeTimeDelta);
    // log::debug("interp timeDelta: {} - {} = {}", newTime, *currentTime, timeDelta);
    // log::info("tdr: {}, td: {}, whole: {}", timeDeltaRatio, timeDelta, wholeTimeDelta);
    
    if (timeDeltaRatio >= 1.0f) {
        // log::debug("extrapolated frame: {}", timeDeltaRatio);
        // extrapolate (unimpl)
        return;
    }

    // log::debug("interp tdr: {}, newTime: {}, oldTime: {}", timeDeltaRatio, newTime, oldTime);

    auto pos = CCPoint{
        std::lerp(cData->olderFrame.pos.x, cData->newerFrame.pos.x, timeDeltaRatio),
        std::lerp(cData->olderFrame.pos.y, cData->newerFrame.pos.y, timeDeltaRatio),
    };

    auto rot = std::lerp(cData->olderFrame.rot, cData->newerFrame.rot, timeDeltaRatio);

    auto frame = PCFrameInfo {
        .pos = pos,
        .rot = rot,
        .timestamp = *currentTime,
    };

    applyFrame(player, frame);
    // log::debug("interp {}", player->getPositionX());
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
            PCFrameInfo {CCPoint{0.f, 0.f}, 0.f, data.timestamp},
            PCFrameInfo {CCPoint{0.f, 0.f}, 0.f, data.timestamp},
        },
        SpecificCorrectionData {
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