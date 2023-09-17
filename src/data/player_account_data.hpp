#pragma once
#include "../net/bytebuffer.hpp"

struct PlayerAccountData {
    int cube, ship, ball, ufo, wave, robot, spider, color1, color2;
    std::string name;
};

void encodeAccountData(const PlayerAccountData& data, ByteBuffer& buffer);
PlayerAccountData decodeAccountData(ByteBuffer& buffer);