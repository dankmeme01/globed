/*
* DRPPAEngine - position approximation using the Dead Reckoning method.
*
* From wikipedia:
* In navigation, dead reckoning is the process of calculating the current position
* of a moving object by using a previously determined position, or fix,
* and incorporating estimates of speed, heading (or direction or course), and elapsed time
*
* (here: frame = a packet dictating position + rotation of another player)
* This engine compares the rotation + position difference between last two frames,
* and essentially "predicts into the future" where the player will be
* in between this frame and the next one.
*/

#pragma once
#include "ppa_base.hpp"

class DRPPAEngine : public PPAEngine {
public:
    // ~DRPPAEngine();
    void updateSpecificPlayer(
        CCSprite* player,
        const SpecificIconData& data,
        float frameDelta,
        int playerId,
        bool isSecond
    ) override;

    void addPlayer(int playerId, const PlayerData& data) override;
    void removePlayer(int playerId) override;

protected:
    std::unordered_map<int, std::pair<CCPoint, CCPoint>> errCorrections;
    // std::unordered_map<int, CCPoint> rotCorrections;
    std::unordered_map<int, std::pair<CCPoint, CCPoint>> lastRealPos;
    // std::unordered_map<int, CCPoint> lastRealRot;
};