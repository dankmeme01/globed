/*
* DisabledPPAEngine - No approximation.
*
* This engine will display the raw data sent by the server.
* That's it. Nothing special, just what you normally expect.
*/

#pragma once
#include "ppa_base.hpp"

class DisabledPPAEngine : public PPAEngine {
public:
    void updateSpecificPlayer(
        CCSprite* player,
        const SpecificIconData& data,
        float frameDelta,
        int playerId,
        bool isSecond
    ) override;
};