/*
* PlayerCorrection is a new version of PPA engines.
* It is made to eliminate lag and jitter caused by unstable ping and packet loss.
* It uses a combination of interpolation, extrapolation and packet timestamping.
*/
#pragma once
#include <Geode/Geode.hpp>
#include "../data/data.hpp"
#include "../ui/remote_player.hpp"

struct PCFrameInfo {
    CCPoint pos;
    float rot, timestamp;
};

struct SpecificCorrectionData {
    PCFrameInfo newerFrame;
    PCFrameInfo olderFrame;
};

struct PlayerCorrectionData {
    float timestamp;
    SpecificCorrectionData p1;
    SpecificCorrectionData p2;
};

class PlayerCorrection {
public:
    void updateSpecificPlayer(
        RemotePlayer* player,
        const SpecificIconData& data,
        float frameDelta,
        int playerId,
        bool isSecond,
        bool isPractice,
        float timestamp
    );

    void updatePlayer(
        std::pair<RemotePlayer*, RemotePlayer*>& player,
        const PlayerData& data,
        float frameDelta,
        int playerId
        // uint64_t timestamp
    );

    void applyFrame(RemotePlayer* player, const PCFrameInfo& data);

    void addPlayer(int playerId, const PlayerData& data);
    void removePlayer(int playerId);
    void setTargetDelta(float dt);
protected:
    float targetUpdateDelay;

    std::unordered_map<int, PlayerCorrectionData> playerData;
    bool firstFrame = true;
    float preservedDashDeltaP1 = 0.f, preservedDashDeltaP2 = 0.f;
};