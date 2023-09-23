#include "corrector.hpp"
#include "../util.hpp"
#include <numeric>

const float DASH_DEGREES_PER_SECOND = 900.f; // this is weird, if too fast use 720.f

PlayerData emptyPlayerData() {
    return PlayerData {
        .timestamp = 0.f,
        .player1 = SpecificIconData {
            .x = 0.f,
            .y = 0.f,
            .rot = 0.f,
            .gameMode = IconGameMode::CUBE,
            .isHidden = true,
            .isDashing = false,
            .isUpsideDown = false,
            .isMini = false,
            .isGrounded = true,
        },
        .player2 = SpecificIconData {
            .x = 0.f,
            .y = 0.f,
            .rot = 0.f,
            .gameMode = IconGameMode::CUBE,
            .isHidden = true,
            .isDashing = false,
            .isUpsideDown = false,
            .isMini = false,
            .isGrounded = true,
        },
        .isPractice = false,
    };
}

bool closeEqual(float f1, float f2) {
    float fmin = f1 - 0.002f;
    float fmax = f1 + 0.002f;
    return f2 > fmin && f2 < fmax;
}

bool closeGreater(float f1, float f2) {
    float fmin = f2 - 0.002f;
    return f1 > fmin;
}

void PlayerCorrector::feedRealData(const std::unordered_map<int, PlayerData>& data) {
    for (const auto& [playerId, data] : data) {
        if (!playerData.contains(playerId)) {
            playersNew.push_back(playerId);
            auto pData = std::make_pair(
                playerId,
                PlayerCorrectionData {
                    .timestamp = data.timestamp,
                    .realTimestamp = data.timestamp,
                    .newerFrame = data,
                    .olderFrame = emptyPlayerData(),
                    .sentPackets = 0,
                    .extrapolatedFrames = 0
                }
            );

            playerData.insert(pData);
        } else {
            auto pData = playerData[playerId].write();
            // if double get the midpoint
            if (closeEqual(data.timestamp - pData->newerFrame.timestamp, targetUpdateDelay * 2) && pData->extrapolatedFrames < 2) {
                auto midPoint = getMidPoint(pData->newerFrame, data);
                log::debug("!! midpoint: between {} and {} = {}, y = {}, t = {}", pData->newerFrame.player1.x, data.player1.x, midPoint.player1.x, midPoint.player1.y, midPoint.timestamp);
                pData->olderFrame = pData->newerFrame;
                pData->newerFrame = midPoint;
                pData->extrapolatedFrames += 1;
            
            // if same as last frame, extrapolate
            } else if (data.timestamp == pData->newerFrame.timestamp && pData->extrapolatedFrames < 2) {
                auto expFrame = getExtrapolatedFrame(pData->olderFrame, pData->newerFrame);
                log::debug("!! extrapolated: between {} and {} = {}, y = {}, t = {}", pData->olderFrame.player1.x, pData->newerFrame.player1.x, expFrame.player1.x, expFrame.player1.y, expFrame.timestamp);
                pData->olderFrame = pData->newerFrame;
                pData->newerFrame = expFrame;
                pData->extrapolatedFrames += 1;

            } else {
                log::debug("updating with: {}, t = {}", data.player1.x, data.timestamp);
                pData->olderFrame = pData->newerFrame;
                pData->newerFrame = data;
                pData->extrapolatedFrames = 0;
            }
            pData->sentPackets += 1;

            if (pData->sentPackets < 60 || pData->sentPackets % 30 == 0) {
                log::debug("syncing timestamp from {} to {}", pData->timestamp, pData->olderFrame.timestamp);
                pData->timestamp = pData->olderFrame.timestamp;
            }
        }
    }

    std::vector<int> toRemove;

    for (const auto& [playerId, _] : playerData) {
        if (!data.contains(playerId)) {
            toRemove.push_back(playerId);
        }
    }

    std::lock_guard lock(mtx);
    for (int id : toRemove) {
        playersGone.push_back(id);
        playerData.erase(id);
    }
}

PlayerData PlayerCorrector::getMidPoint(const PlayerData& older, const PlayerData& newer) {
    PlayerData out = newer;
    out.timestamp = std::midpoint(older.timestamp, newer.timestamp);
    out.player1.x = std::midpoint(older.player1.x, newer.player1.x);
    out.player1.y = std::midpoint(older.player1.y, newer.player1.y);
    out.player1.rot = std::midpoint(older.player1.rot, newer.player1.rot);

    out.player2.x = std::midpoint(older.player2.x, newer.player2.x);
    out.player2.y = std::midpoint(older.player2.y, newer.player2.y);
    out.player2.rot = std::midpoint(older.player2.rot, newer.player2.rot);

    return out;
}

PlayerData PlayerCorrector::getExtrapolatedFrame(const PlayerData& older, const PlayerData& newer) {
    PlayerData out = newer;
    // ptr = normally 1, if we had a packet loss then may be 0.5, 0.33 or less
    auto passedTimeRatio = targetUpdateDelay / (newer.timestamp - older.timestamp);
    out.timestamp = std::lerp(older.timestamp, newer.timestamp, 1.f + passedTimeRatio);
    out.player1.x = std::lerp(older.player1.x, newer.player1.x, 1.f + passedTimeRatio);
    out.player1.y = std::lerp(older.player1.y, newer.player1.y, 1.f + passedTimeRatio);
    out.player1.rot = std::lerp(older.player1.rot, newer.player1.rot, 1.f + passedTimeRatio);

    out.player2.x = std::lerp(older.player2.x, newer.player2.x, 1.f + passedTimeRatio);
    out.player2.y = std::lerp(older.player2.y, newer.player2.y, 1.f + passedTimeRatio);
    out.player2.rot = std::lerp(older.player2.rot, newer.player2.rot, 1.f + passedTimeRatio);

    return out;
}

void PlayerCorrector::interpolate(const std::pair<RemotePlayer*, RemotePlayer*>& player, float frameDelta, int playerId) {
    std::lock_guard lock(mtx);
    if (!playerData.contains(playerId)) {
        return;
    }

    interpolateSpecific(player.first, frameDelta, playerId, false);
    interpolateSpecific(player.second, frameDelta, playerId, true);

    playerData.at(playerId).write()->timestamp += frameDelta;
}

void PlayerCorrector::interpolateSpecific(RemotePlayer* player, float frameDelta, int playerId, bool isSecond) {
    auto data = playerData.at(playerId).read();
    auto& olderData = isSecond ? data->olderFrame.player2 : data->olderFrame.player1;
    auto& newerData = isSecond ? data->newerFrame.player2 : data->newerFrame.player1;
    auto gamemode = newerData.gameMode;

    if (!newerData.isHidden) {
        player->setVisible(true);
        auto scale = newerData.isMini ? 0.6f : 1.0f;
        player->setScale(scale);

        if (gamemode != IconGameMode::CUBE) { // cube reacts differently to m_isUpsideDown i think
            player->setScaleY(scale * (newerData.isUpsideDown ? -1 : 1));
        }
        
        player->tick(newerData, data->newerFrame.isPractice);
    } else {
        player->setVisible(false);
        return;
    }

    auto olderTime = data->olderFrame.timestamp;
    auto newerTime = data->newerFrame.timestamp;
    
    // on higher pings targetUpdateDelay works like a charm,
    // the other one is a laggy mess

    // auto wholeTimeDelta = newerTime - olderTime;
    auto wholeTimeDelta = targetUpdateDelay;

    if (closeEqual(wholeTimeDelta, 0.f)) {
        log::debug("skipping wholeTimeDelta = 0");
        wholeTimeDelta = targetUpdateDelay;
        // return;
    }

    auto targetDelayIncrement = frameDelta / targetUpdateDelay;

    float currentTime = data->timestamp;
    if (currentTime < olderTime) {
        currentTime = currentTime + wholeTimeDelta;
    }
    float timeDelta = currentTime - olderTime;
    float timeDeltaRatio = timeDelta / wholeTimeDelta;

    auto pos = CCPoint {
        std::lerp(olderData.x, newerData.x, timeDeltaRatio),
        std::lerp(olderData.y, newerData.y, timeDeltaRatio),
    };

    if (gamemode == IconGameMode::SPIDER && newerData.isGrounded) {
        pos.y = olderData.y;
    }

    float rot = newerData.rot;

    if (newerData.isDashing && (gamemode == IconGameMode::CUBE || gamemode == IconGameMode::BALL)) {
        float dashDelta = DASH_DEGREES_PER_SECOND * wholeTimeDelta;
        if (newerData.isUpsideDown) {
            dashDelta *= -1;
        }

        auto base = player->getRotation();
        rot = base + std::lerp(0, dashDelta, timeDeltaRatio);
    } else {
        // disable rot interp if the spin is too big (such as from 580 degrees to -180).
        // dont ask me why that can happen, just gd moment.
        if (abs(newerData.rot - olderData.rot) < 360) {
            rot = std::lerp(olderData.rot, newerData.rot, timeDeltaRatio);
        }
    }

    log::debug("ts = {}, tdr = {}, x: {} <-> {} = {}, y: {} <-> {} = {}", timeDelta, timeDeltaRatio, olderData.x, newerData.x, pos.x, olderData.y, newerData.y, pos.y);
    if (timeDeltaRatio < 0.f || timeDeltaRatio > 5.f) {
        // data->tryCorrectTimestamp = 5;
    }

    player->setPosition(pos);
    player->setRotation(rot);
}

void PlayerCorrector::setTargetDelta(float dt) {
    targetUpdateDelay = dt;
}

void PlayerCorrector::joinedLevel() {
    playerData.clear();
    playersGone.clear();
    playersNew.clear();
}

std::vector<int> PlayerCorrector::getPlayerIds() {
    return globed_util::mapKeys(playerData);
}

std::vector<int> PlayerCorrector::getNewPlayers() {
    auto copy = playersNew;
    playersNew.clear();
    return copy;
}

std::vector<int> PlayerCorrector::getGonePlayers() {
    auto copy = playersGone;
    playersGone.clear();
    return copy;
}