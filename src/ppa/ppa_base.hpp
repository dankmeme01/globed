#pragma once
#include <Geode/Geode.hpp>
#include "../data/data.hpp"

using namespace geode::prelude;

class PPAEngine {
public:
    virtual ~PPAEngine();
    virtual void updateSpecificPlayer(
        CCSprite* player,
        const SpecificIconData& data,
        float frameDelta,
        int playerId,
        bool isSecond
    ) = 0;
    virtual void updatePlayer(
        std::pair<CCSprite*, CCSprite*>& player,
        const PlayerData& data,
        float frameDelta,
        int playerId
    );
    virtual void updateHiddenPlayer(
        CCSprite* player,
        const SpecificIconData& data,
        float frameDelta,
        int playerId,
        bool isSecond
    );

    virtual void addPlayer(int playerId, const PlayerData& data);
    virtual void removePlayer(int playerId);

    void setTargetDelta(float dt);
protected:
    float targetUpdateDelay;
};