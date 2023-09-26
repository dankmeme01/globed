#pragma once
#include "player_progress.hpp"

/*
* PlayerProgressNew subclass PlayerProgress so either can be stored in the same variable.
*/

class PlayerProgressNew : public PlayerProgress {
protected:
    bool init(int playerId_);
public:
    void updateValues(float percentage, bool onRightSide);
    static PlayerProgressNew* create(int playerId_);
};