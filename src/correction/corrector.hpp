#pragma once
#include <Geode/Geode.hpp>
#include "../data/data.hpp"
#include "../ui/remote_player.hpp"

struct PlayerCorrectionData {
    float timestamp;
    PlayerData newerFrame;
    PlayerData olderFrame;
    int sentPackets;
    bool tryCorrectTimestamp;
};

class PlayerCorrector {
public:
    void feedRealData(const std::unordered_map<int, PlayerData>& data);
    void interpolate(const std::pair<RemotePlayer*, RemotePlayer*>& player, float frameDelta, int playerId);
    void interpolateSpecific(RemotePlayer* player, float frameDelta, int playerId, bool isSecond);
    void setTargetDelta(float dt);
    void joinedLevel();

    std::vector<int> getPlayerIds();
    std::vector<int> getNewPlayers();
    std::vector<int> getGonePlayers();

protected:
    float targetUpdateDelay;
    std::unordered_map<int, PlayerCorrectionData> playerData;
    std::vector<int> playersGone;
    std::vector<int> playersNew;
    std::mutex mtx;
};