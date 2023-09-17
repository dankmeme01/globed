/*
* InterpolationPPAEngine - position approximation with the interpolation method.
*
* This engine is similar to Dead Reckoning, except it sacrifices
* an extra frame of latency in order to achieve higher accuracy.
*
* (here: frame = a packet dictating position + rotation of another player)
* When a frame is received, rather than immediately drawing it,
* we store it in a temporary variable. Then, when we get a new frame,
* we compare these two and insert frames **in between them**.
* This is in contrast to Dead Reckoning, which would insert frames **afterwards**
*
* Because of that, the accuracy is greatly improved.
* However this comes with a drawback of rendering other players
* one frame later than they normally would be.
* In the real world, the 1 frame difference is insignificant,
* while the increased accuracy is always welcome, so this method is the default.
*/

#pragma once
#include "ppa_base.hpp"

class InterpolationPPAEngine : public PPAEngine {
public:
    void updateSpecificPlayer(
        RemotePlayer* player,
        const SpecificIconData& data,
        float frameDelta,
        int playerId,
        bool isSecond
    ) override;

    void addPlayer(int playerId, const PlayerData& data) override;
    void removePlayer(int playerId) override;

protected:
    using PosInfo = std::pair<CCPoint, CCPoint>; // pos, rot
    std::unordered_map<int, PosInfo> lastFrameP1, lastFrameP2;
    std::unordered_map<int, PosInfo> preLastFrameP1, preLastFrameP2;
    bool firstFrame = true;
    float preservedDashDeltaP1 = 0.f, preservedDashDeltaP2 = 0.f;
};