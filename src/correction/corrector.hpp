#pragma once
#include <Geode/Geode.hpp>
#include "../data/data.hpp"
#include "../ui/remote_player.hpp"
#include "../wrapping_rwlock.hpp"

struct PlayerCorrectionData {
    float timestamp;
    float realTimestamp;
    PlayerData newerFrame;
    PlayerData olderFrame;
    int sentPackets;
    int extrapolatedFrames;
};

class PlayerCorrector {
public:
    void feedRealData(const std::unordered_map<int, PlayerData>& data);
    void interpolate(const std::pair<RemotePlayer*, RemotePlayer*>& player, float frameDelta, int playerId);
    void interpolateSpecific(RemotePlayer* player, float frameDelta, int playerId, bool isSecond);
    void setTargetDelta(float dt);
    void joinedLevel();
    PlayerData getMidPoint(const PlayerData& older, const PlayerData& newer);
    PlayerData getExtrapolatedFrame(const PlayerData& older, const PlayerData& newer);

    std::vector<int> getPlayerIds();
    std::vector<int> getNewPlayers();
    std::vector<int> getGonePlayers();

protected:
    float targetUpdateDelay;
    std::unordered_map<int, WrappingRwLock<PlayerCorrectionData>> playerData;
    std::vector<int> playersGone;
    std::vector<int> playersNew;
    std::mutex mtx;
};